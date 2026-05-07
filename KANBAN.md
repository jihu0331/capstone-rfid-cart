# 작업 보드 (KANBAN)

작업이 끝나면 해당 항목 `[ ]` → `[x]`로 표시하고 옆에 비고와 완료일을 적는다.

표기: `- [x] 항목 — 비고 _(YYYY-MM-DD)_`

---

## 1주차 — 부품 확보 / RFID·상품 DB

- [ ] NTAG213 스티커 10장 발주 — 쿠팡 10장 2,600원, NTAG213 / 13.56MHz / 일반형 확인
- [ ] Arduino Uno + RC522 + I2C LCD + 로터리 엔코더 + 부저 부품 수급 확인
- [ ] 부품 도착 후 RC522 단독 통신 확인 (`rfid.PCD_DumpVersionToSerial()` 또는 부팅 로그)
- [ ] `uid-reader.ino` 업로드 → 5장 NTAG213 UID 수집
- [ ] 수집한 UID로 `capstone-rfid-cart.ino` 의 `PRODUCTS[]` 표 5종 매핑 (사과/우유/초콜릿/컵라면/생수)
- [ ] 데모 상품 5종 + 스티커 부착 매핑표 사진/문서화
- [x] NTAG213 스티커 사양 검토 — RC522 호환(13.56MHz / ISO 14443A), UID만 사용해 NTAG213/215/216 모두 가능, 금속 부착 X _(2026-05-07)_
- [x] 쿠팡 10장 2,600원 상품 가격 적정성 판정 — 장당 260원, 시중 평균 이하. 페이지는 봇 차단으로 자동 조회 불가, 사용자 확인 후 OK _(2026-05-07)_
- [x] 1주차 보조 스케치 작성 (`firmware/uid-reader/uid-reader.ino`) — 바이트 출력 + `0xXXXXXXXXUL` 리터럴 + 카드 종류 표시, Flash 18% / SRAM 14% _(2026-05-07)_

---

## 2주차 — 장바구니 / 입력 / LCD / 결제

### 펌웨어 작성 (`firmware/capstone-rfid-cart/capstone-rfid-cart.ino`)
- [x] 핀 배치 결정 — RC522(SS=10, RST=9, SPI 11/12/13), LCD I2C(A4/A5, 0x27), 엔코더(CLK=2 INT0, DT=4, SW=3), 부저(D6) _(2026-05-07)_
- [x] 상품 DB 룩업 테이블 — `PRODUCTS[]` 5종, UID는 placeholder `0x00000001~5UL` _(2026-05-07)_
- [x] 장바구니 자료구조 + 함수 — `CartItem cart[8]`, addOrIncrement / removeCartAt / cartTotal / findCart _(2026-05-07)_
- [x] 로터리 엔코더 인터럽트 — `attachInterrupt(CLK, FALLING)` + DT 읽어 방향 판정 _(2026-05-07)_
- [x] 엔코더 버튼 디바운싱 + short/long press — debounce 30ms, long 800ms _(2026-05-07)_
- [x] 4-state 상태머신 — SCAN / ITEM / PAY_CONFIRM / PAID, ITEM 6초 타임아웃 _(2026-05-07)_
- [x] LCD 화면 4종 렌더 함수 — drawScan / drawItem / drawPayConfirm / drawPaid, 16자 고정폭 _(2026-05-07)_
- [x] 부저 피드백 패턴 — beepOk(1500Hz) / beepEnter(2000Hz) / beepError(400Hz) / beepPaid(3음 멜로디) _(2026-05-07)_
- [x] RFID 스캔 → 상품 매칭 → 카트 추가/수량 증가 흐름 — 미등록 UID는 부저 경고 + Serial 출력 _(2026-05-07)_
- [x] 결제 흐름 — Yes/No 토글, `isPaid = true` 전이 (3주차 게이트가 검사) _(2026-05-07)_

### 펌웨어 버그 수정
- [x] 엔코더 ISR 로직 오류 — CHANGE 인터럽트로는 `00→01` 그레이코드 전이가 발생 안 함 → FALLING + DT 읽기 방식으로 변경 _(2026-05-07)_
- [x] 합계 오버플로우 — 99qty × 3500원만으로도 uint16_t 한계 초과 → `cartTotal()` uint32_t로, snprintf `%u` → `%lu` _(2026-05-07)_
- [x] enum 자동 프로토타입 충돌 — Arduino IDE 전처리기가 enum보다 먼저 프로토타입 삽입 → `enum State` / `enum BtnEvent`를 파일 상단으로 이동 _(2026-05-07)_

### 빌드 환경
- [x] arduino-cli 컴파일 검증 — IDE 번들 cli 사용, capstone-rfid-cart Flash 12,040/32,256(37%) SRAM 758/2048(37%) 통과 _(2026-05-07)_
- [x] PlatformIO 환경 설정 — `platformio.ini`에 env:cart / env:uid-reader 두 환경, `src_dir = firmware` + `build_src_filter`로 분리, lib_deps 선언 _(2026-05-07)_

### 하드웨어 통합 테스트 (부품 도착 후)
- [ ] I2C LCD 주소 스캔 (0x27 / 0x3F) — 다르면 코드 상단 `LiquidCrystal_I2C lcd(0x27, ...)` 수정
- [ ] 로터리 엔코더 방향 검증 — 시계방향이 +1로 나오는지, 반대면 `encISR` 부등호 교체
- [ ] 엔코더 버튼 short/long press 임계값(800ms) 사용감 확인
- [ ] 5종 스캔 → 같은 태그 재스캔 = 수량 +1 동작 확인
- [ ] ITEM 화면에서 수량 0 만들고 short press → 항목 삭제 확인
- [ ] ITEM 화면 long press → 강제 삭제 확인
- [ ] 합계 표시 정확성 확인 (가격 × 수량 누적)
- [ ] 결제 진입 → Yes/No 토글 → 결제 완료 부저 멜로디 확인
- [ ] 카트 가득(8종 초과) 시 에러 부저 + 무시 동작 확인

---

## 3주차 — IR 게이트 / 릴레이 / 서보

- [ ] IR 센서 임계값 캘리브레이션 (사람 통과 거리 기준)
- [ ] 릴레이 ON/OFF 토글 테스트
- [ ] 서보모터 0°/90° 게이트 동작
- [ ] 메인 스케치에 IR/릴레이/서보 통합 (`isPaid` 검사 → 게이트 오픈)
- [ ] 미결제 통과 시 부저 경고 + 게이트 닫힘 유지
- [ ] PAID 상태 종료 조건을 게이트 통과로 변경 (현재는 3초 타임아웃)

---

## 4주차 — 회귀 테스트 / 시연 준비

- [ ] 정상 결제 시나리오 end-to-end 회귀 테스트
- [ ] 미결제 통과 알람 검증
- [ ] 엣지 케이스 — 미등록 UID, 카트 가득, 결제 후 추가 스캔 시도
- [ ] 데모 상품 5종 NTAG213 스티커 최종 부착 + 라벨링
- [ ] 시연 시나리오 리허설 (3분 분량)
- [ ] 발표 자료 정리 (PPT 작성 여부는 막바지에 결정)

---

## 프로젝트 셋업 (공통)

- [x] 초기 프로젝트 스캐폴드 — README/CLAUDE/PLAN/KANBAN/.gitignore, 첫 커밋 `b614dd4` _(2026-05-07)_
- [x] PLAN.md 주차 구성 재편 — 교수님 안내(1주차 RFID/DB, 2주차 장바구니/결제, 3주차 게이트, 4주차 시연)에 맞춰 마일스톤 재배치, 변경 로그에 기록 _(2026-05-07)_
- [x] GitHub Public 저장소 생성 및 푸시 — https://github.com/jihu0331/capstone-rfid-cart, `gh repo create --public --push` _(2026-05-07)_
- [x] 교수님 collaborator 초대 방식 결정 — Public 저장소이므로 URL 공유로 열람 가능, 권한 필요 시 GitHub 웹 UI에서 hongjoo.lee@snu.ac.kr 입력 _(2026-05-07)_
