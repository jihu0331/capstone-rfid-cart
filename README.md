# RFID 기반 스마트 쇼핑카트 시스템

사물인터넷 / 창의공학설계 캡스톤 프로젝트

- **학생**: 김지후 (2020-14270)
- **지도교수**: hongjoo.lee@snu.ac.kr

## 개요
RC522 RFID 리더와 NTAG213 NFC 스티커를 이용해 상품을 자동 인식하고, 결제 완료 여부에 따라 게이트를 통과시키는 무인 쇼핑카트 시스템.

## 하드웨어
- Arduino Uno
- RC522 RFID 리더 + NTAG213 스티커
- I2C LCD 16x2
- 로터리 엔코더 (버튼 포함)
- IR 센서, 릴레이, 서보모터
- 부저

## 동작 흐름
1. 상품 스캔 → EEPROM DB 매칭
2. 장바구니 관리 (수량/취소)
3. 합계 LCD 표시
4. 엔코더 버튼으로 결제 (`isPaid = true`)
5. 게이트 IR 센서 트리거 → 결제 검사
6. 릴레이 ON → 서보모터 게이트 오픈

## 문서
- [PLAN.md](./PLAN.md) — 주차별 구현 계획 및 변경 로그
- [KANBAN.md](./KANBAN.md) — 작업 보드 (To-do / Doing / Done)
- [CLAUDE.md](./CLAUDE.md) — Claude Code 세션 컨텍스트
