// NTAG213 UID 리더 (1주차 보조 스케치)
// 태그를 가져다 대면 Serial Monitor에 UID를 출력한다.
// 출력 형식은 capstone-rfid-cart.ino 의 PRODUCTS[] 표에 그대로 붙여 넣을 수 있도록
// "0xXXXXXXXXUL" 리터럴로도 보여준다.
//
// 사용법:
//   1) Arduino IDE에서 이 스케치를 업로드 (보드: Arduino Uno).
//   2) Serial Monitor 열기, 보레이트 9600.
//   3) NTAG213 스티커를 RC522에 차례로 가져다 댄다.
//   4) 화면에 찍힌 "0x........UL" 값을 메인 스케치 PRODUCTS[] 표의 UID 칸에 복사.
//
// 배선 (메인 스케치와 동일):
//   RC522: SDA(SS)=10, RST=9, SCK=13, MOSI=11, MISO=12, 3.3V, GND

#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t PIN_SS  = 10;
constexpr uint8_t PIN_RST = 9;

MFRC522 rfid(PIN_SS, PIN_RST);

uint32_t lastUid = 0;
unsigned long lastReadAt = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }
  SPI.begin();
  rfid.PCD_Init();

  Serial.println(F("=== NTAG213 UID Reader ==="));
  Serial.println(F("태그를 RC522에 가져다 대세요."));
  Serial.println(F("출력 형식: [bytes] | [32-bit literal]"));
  Serial.println();
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent())   return;
  if (!rfid.PICC_ReadCardSerial())     return;

  // 1) 원본 바이트 출력
  Serial.print(F("UID bytes: "));
  for (uint8_t i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
    Serial.print(rfid.uid.uidByte[i], HEX);
    if (i + 1 < rfid.uid.size) Serial.print(' ');
  }

  // 2) 메인 스케치에서 사용하는 32-bit 리터럴
  uint32_t uid32 = 0;
  for (uint8_t i = 0; i < rfid.uid.size && i < 4; i++) {
    uid32 = (uid32 << 8) | rfid.uid.uidByte[i];
  }
  Serial.print(F("   |   literal: 0x"));
  for (int8_t shift = 24; shift >= 0; shift -= 8) {
    uint8_t b = (uid32 >> shift) & 0xFF;
    if (b < 0x10) Serial.print('0');
    Serial.print(b, HEX);
  }
  Serial.print(F("UL"));

  // 3) 카드 종류 (참고용)
  MFRC522::PICC_Type type = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print(F("   ("));
  Serial.print(rfid.PICC_GetTypeName(type));
  Serial.println(F(")"));

  // 같은 태그 연속 인식 시 1초 간격으로만 새로 출력
  if (uid32 != lastUid || millis() - lastReadAt > 1000UL) {
    lastUid = uid32;
    lastReadAt = millis();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(200);
}
