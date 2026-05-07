# RFID 기반 스마트 쇼핑카트 시스템

## 프로젝트 정체성
- **과목**: 사물인터넷 / 창의공학설계 캡스톤
- **학생**: 김지후 (2020-14270)
- **지도교수**: hongjoo.lee@snu.ac.kr
- **저장소**: 로컬 git, 추후 GitHub로 푸시 (교수 초대 필요)

## 하드웨어 구성
| 카테고리 | 부품 |
|---|---|
| MCU | Arduino Uno |
| RFID | RC522 + NTAG213 스티커 |
| 디스플레이 | I2C LCD 16x2 |
| 입력 | 로터리 엔코더 (버튼 포함) |
| 알림 | 부저 |
| 게이트 | IR 센서 + 릴레이 + 서보모터 |

## 동작 시나리오 (6단계)
1. 상품 스캔 (RC522 → EEPROM DB)
2. 장바구니 관리 (수량/취소)
3. 합계 계산 (LCD 표시)
4. 결제 처리 (엔코더 버튼, `isPaid = true`)
5. 게이트 통과 감지 (IR → `isPaid` 검사)
6. 게이트 오픈 (릴레이 → 서보)

## 핵심 상태 변수 (예상)
- `cart[]`: { tagId, name, price, qty }
- `total`: 누적 금액
- `isPaid`: 결제 완료 플래그
- `gateOpen`: 게이트 상태

## 작업 규약
- **모든 계획 변경/실패는 [PLAN.md](./PLAN.md) 변경 로그에 기록** (시간 X, 밀린 일기 형식)
- **작업 단위는 [KANBAN.md](./KANBAN.md)에서 To-do → Doing → Done으로 이동**
- 발표용 PPT 생성은 보류 (4주차 마무리 단계)

## 작업 환경
- 로컬 폴더: `C:\Users\user\dev\capstone-rfid-cart\`
- 플랫폼: Windows 11, Arduino IDE / PlatformIO
