// RFID 기반 스마트 쇼핑카트 - 2주차 통합 스케치
// 기능: RFID 스캔 → 상품 매칭 → 장바구니 → 엔코더 수량 조절 → LCD 표시 → 결제
// 게이트(IR/릴레이/서보)는 3주차에서 추가.
//
// 필요 라이브러리:
//   - MFRC522                (by GithubCommunity)
//   - LiquidCrystal_I2C      (by Frank de Brabander)
//
// 배선:
//   RC522:   SDA(SS)=10, RST=9, SCK=13, MOSI=11, MISO=12, 3.3V, GND
//   LCD I2C: SDA=A4, SCL=A5, 5V, GND  (주소 0x27)
//   엔코더:  CLK=D2(INT0), DT=D4, SW=D3(INT1), +=5V, GND
//   부저:    D6, GND

#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

// ───── 핀 정의 ─────
constexpr uint8_t PIN_SS    = 10;
constexpr uint8_t PIN_RST   = 9;
constexpr uint8_t PIN_CLK   = 2;   // 엔코더 A
constexpr uint8_t PIN_DT    = 4;   // 엔코더 B
constexpr uint8_t PIN_SW    = 3;   // 엔코더 버튼
constexpr uint8_t PIN_BUZZ  = 6;

// ───── 타입 (Arduino IDE 자동 프로토타입보다 먼저 보여야 함) ─────
enum State    : uint8_t { STATE_SCAN, STATE_ITEM, STATE_PAY_CONFIRM, STATE_PAID };
enum BtnEvent : uint8_t { BTN_NONE, BTN_SHORT, BTN_LONG };

// ───── 객체 ─────
MFRC522            rfid(PIN_SS, PIN_RST);
LiquidCrystal_I2C  lcd(0x27, 16, 2);

// ───── 상품 DB ─────
// 1주차에서 실제 NTAG213 UID를 읽어 이 표를 채운다.
struct Product { uint32_t uid; const char* name; uint16_t price; };
const Product PRODUCTS[] = {
  { 0x00000001UL, "Apple",     1500 },
  { 0x00000002UL, "Milk",      2800 },
  { 0x00000003UL, "Chocolate", 3500 },
  { 0x00000004UL, "Cup Ramen", 1200 },
  { 0x00000005UL, "Water",      900 },
};
constexpr uint8_t PRODUCT_COUNT = sizeof(PRODUCTS) / sizeof(PRODUCTS[0]);

// ───── 장바구니 ─────
constexpr uint8_t CART_MAX = 8;
struct CartItem { uint8_t productIdx; uint8_t qty; };
CartItem cart[CART_MAX];
uint8_t  cartSize = 0;

uint32_t cartTotal() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < cartSize; i++) {
    sum += (uint32_t)PRODUCTS[cart[i].productIdx].price * cart[i].qty;
  }
  return sum;
}

int8_t findCart(uint8_t productIdx) {
  for (uint8_t i = 0; i < cartSize; i++) {
    if (cart[i].productIdx == productIdx) return i;
  }
  return -1;
}

int8_t addOrIncrement(uint8_t productIdx) {
  int8_t idx = findCart(productIdx);
  if (idx >= 0) {
    if (cart[idx].qty < 99) cart[idx].qty++;
    return idx;
  }
  if (cartSize >= CART_MAX) return -1;  // 가득 참
  cart[cartSize] = { productIdx, 1 };
  return cartSize++;
}

void removeCartAt(uint8_t idx) {
  if (idx >= cartSize) return;
  for (uint8_t i = idx; i + 1 < cartSize; i++) cart[i] = cart[i + 1];
  cartSize--;
}

void clearCart() { cartSize = 0; }

// ───── 상태머신 ─────
State          state           = STATE_SCAN;
bool           isPaid          = false;       // 3주차 게이트가 검사
int8_t         selectedCartIdx = -1;
bool           payConfirmYes   = true;
unsigned long  stateEnteredAt  = 0;
bool           needsRedraw     = true;

constexpr unsigned long ITEM_TIMEOUT_MS = 6000UL;
constexpr unsigned long PAID_DISPLAY_MS = 3000UL;

void enterState(State s) {
  state = s;
  stateEnteredAt = millis();
  needsRedraw = true;
}

// ───── 엔코더 (인터럽트 + 버튼) ─────
// CLK 하강 에지에서 DT를 읽어 방향 판정 (디텐트당 1스텝).
volatile int8_t encDelta = 0;

void encISR() {
  if (digitalRead(PIN_DT) == HIGH) encDelta++;
  else                              encDelta--;
}

int8_t readEncoder() {
  noInterrupts();
  int8_t d = encDelta;
  encDelta = 0;
  interrupts();
  return d;
}

// ───── 버튼 (debounce + short/long press) ─────
constexpr unsigned long DEBOUNCE_MS  = 30UL;
constexpr unsigned long LONGPRESS_MS = 800UL;
bool           btnLastStable   = HIGH;
bool           btnLastReading  = HIGH;
unsigned long  btnLastChange   = 0;
unsigned long  btnDownAt       = 0;
bool           btnLongFired    = false;

BtnEvent readButton() {
  BtnEvent ev = BTN_NONE;
  bool reading = digitalRead(PIN_SW);
  unsigned long now = millis();

  if (reading != btnLastReading) {
    btnLastChange = now;
    btnLastReading = reading;
  }

  if (now - btnLastChange > DEBOUNCE_MS && reading != btnLastStable) {
    btnLastStable = reading;
    if (btnLastStable == LOW) {
      btnDownAt = now;
      btnLongFired = false;
    } else {
      // 떼는 순간: long이 아직 안 나갔다면 short
      if (!btnLongFired) ev = BTN_SHORT;
    }
  }

  // 누르고 있는 동안 long press 체크
  if (btnLastStable == LOW && !btnLongFired && now - btnDownAt >= LONGPRESS_MS) {
    btnLongFired = true;
    ev = BTN_LONG;
  }

  return ev;
}

// ───── 부저 ─────
void beepOk()    { tone(PIN_BUZZ, 1500, 80); }
void beepEnter() { tone(PIN_BUZZ, 2000, 50); }
void beepError() { tone(PIN_BUZZ, 400,  200); }
void beepPaid()  {
  tone(PIN_BUZZ, 1200, 100); delay(120);
  tone(PIN_BUZZ, 1600, 100); delay(120);
  tone(PIN_BUZZ, 2000, 200);
}

// ───── RFID 헬퍼 ─────
uint32_t readUID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return 0;
  uint32_t uid = 0;
  for (uint8_t i = 0; i < rfid.uid.size && i < 4; i++) {
    uid = (uid << 8) | rfid.uid.uidByte[i];
  }
  rfid.PICC_HaltA();
  return uid;
}

int8_t lookupProduct(uint32_t uid) {
  for (uint8_t i = 0; i < PRODUCT_COUNT; i++) {
    if (PRODUCTS[i].uid == uid) return i;
  }
  return -1;
}

// ───── LCD 화면 ─────
void lcdLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  char buf[17];
  uint8_t i = 0;
  for (; text[i] && i < 16; i++) buf[i] = text[i];
  for (; i < 16; i++) buf[i] = ' ';
  buf[16] = 0;
  lcd.print(buf);
}

void drawScan() {
  char line0[17];
  snprintf(line0, sizeof(line0), "Scan tag  %dpcs", cartSize);
  lcdLine(0, line0);
  char line1[17];
  snprintf(line1, sizeof(line1), "Total: %lu KRW", (unsigned long)cartTotal());
  lcdLine(1, line1);
}

void drawItem() {
  if (selectedCartIdx < 0 || selectedCartIdx >= (int8_t)cartSize) {
    enterState(STATE_SCAN);
    return;
  }
  const CartItem& ci = cart[selectedCartIdx];
  const Product&  p  = PRODUCTS[ci.productIdx];
  char line0[17];
  snprintf(line0, sizeof(line0), "%-10s x%2u", p.name, ci.qty);
  lcdLine(0, line0);
  char line1[17];
  uint32_t sub = (uint32_t)p.price * ci.qty;
  snprintf(line1, sizeof(line1), "%lu KRW rot/btn", (unsigned long)sub);
  lcdLine(1, line1);
}

void drawPayConfirm() {
  char line0[17];
  snprintf(line0, sizeof(line0), "Pay %lu KRW?", (unsigned long)cartTotal());
  lcdLine(0, line0);
  lcdLine(1, payConfirmYes ? ">YES   no      " : " yes  >NO      ");
}

void drawPaid() {
  lcdLine(0, "*** PAID ***");
  char line1[17];
  snprintf(line1, sizeof(line1), "%lu KRW Thanks!", (unsigned long)cartTotal());
  lcdLine(1, line1);
}

void redraw() {
  switch (state) {
    case STATE_SCAN:        drawScan();        break;
    case STATE_ITEM:        drawItem();        break;
    case STATE_PAY_CONFIRM: drawPayConfirm();  break;
    case STATE_PAID:        drawPaid();        break;
  }
  needsRedraw = false;
}

// ───── 상태별 처리 ─────
void handleScan(int8_t enc, BtnEvent btn) {
  // 새 태그
  uint32_t uid = readUID();
  if (uid != 0) {
    int8_t pIdx = lookupProduct(uid);
    if (pIdx < 0) {
      Serial.print(F("[unknown UID] 0x")); Serial.println(uid, HEX);
      beepError();
      return;
    }
    int8_t cIdx = addOrIncrement(pIdx);
    if (cIdx < 0) { beepError(); return; }   // 카트 가득
    selectedCartIdx = cIdx;
    beepOk();
    enterState(STATE_ITEM);
    return;
  }

  // 엔코더 회전: 카트 항목 사이 이동 (편집 진입 준비)
  if (enc != 0 && cartSize > 0) {
    if (selectedCartIdx < 0) selectedCartIdx = 0;
    selectedCartIdx = (selectedCartIdx + enc + cartSize) % cartSize;
    enterState(STATE_ITEM);
    return;
  }

  // 짧게 누름: 결제 진입
  if (btn == BTN_SHORT) {
    if (cartSize == 0) { beepError(); return; }
    payConfirmYes = true;
    beepEnter();
    enterState(STATE_PAY_CONFIRM);
  }
}

void handleItem(int8_t enc, BtnEvent btn) {
  if (selectedCartIdx < 0 || selectedCartIdx >= (int8_t)cartSize) {
    enterState(STATE_SCAN); return;
  }

  if (enc != 0) {
    int16_t q = (int16_t)cart[selectedCartIdx].qty + enc;
    if (q < 0)  q = 0;
    if (q > 99) q = 99;
    cart[selectedCartIdx].qty = (uint8_t)q;
    stateEnteredAt = millis();   // 활동 갱신
    needsRedraw = true;
    return;
  }

  if (btn == BTN_SHORT) {
    // 수량 0이면 제거
    if (cart[selectedCartIdx].qty == 0) removeCartAt(selectedCartIdx);
    selectedCartIdx = -1;
    beepOk();
    enterState(STATE_SCAN);
    return;
  }

  if (btn == BTN_LONG) {
    // 길게 누름 = 강제 삭제
    removeCartAt(selectedCartIdx);
    selectedCartIdx = -1;
    beepOk();
    enterState(STATE_SCAN);
    return;
  }

  if (millis() - stateEnteredAt > ITEM_TIMEOUT_MS) {
    selectedCartIdx = -1;
    enterState(STATE_SCAN);
  }
}

void handlePayConfirm(int8_t enc, BtnEvent btn) {
  if (enc != 0) {
    payConfirmYes = !payConfirmYes;
    needsRedraw = true;
  }
  if (btn == BTN_SHORT) {
    if (payConfirmYes) {
      isPaid = true;
      beepPaid();
      enterState(STATE_PAID);
    } else {
      beepOk();
      enterState(STATE_SCAN);
    }
  }
}

void handlePaid(int8_t /*enc*/, BtnEvent /*btn*/) {
  // 3주차에서는 게이트 통과까지 isPaid를 유지.
  // 2주차 단독 테스트 단계에서는 화면만 잠깐 보여주고 초기화.
  if (millis() - stateEnteredAt > PAID_DISPLAY_MS) {
    clearCart();
    selectedCartIdx = -1;
    isPaid = false;             // 3주차 통합 시 이 줄은 게이트 모듈로 이동
    enterState(STATE_SCAN);
  }
}

// ───── setup / loop ─────
void setup() {
  Serial.begin(9600);

  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);
  pinMode(PIN_BUZZ, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), encISR, FALLING);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcdLine(0, "Smart Cart v0.2");
  lcdLine(1, "Booting...");

  SPI.begin();
  rfid.PCD_Init();

  delay(500);
  enterState(STATE_SCAN);
  Serial.println(F("Ready."));
}

void loop() {
  int8_t   enc = readEncoder();
  BtnEvent btn = readButton();

  switch (state) {
    case STATE_SCAN:        handleScan(enc, btn);        break;
    case STATE_ITEM:        handleItem(enc, btn);        break;
    case STATE_PAY_CONFIRM: handlePayConfirm(enc, btn);  break;
    case STATE_PAID:        handlePaid(enc, btn);        break;
  }

  if (needsRedraw) redraw();
}
