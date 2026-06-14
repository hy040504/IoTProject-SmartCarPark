# 스마트 주차장 프로젝트

아두이노와 Node.js로 구성한 스마트 주차장 실습 프로젝트다. 현재 구조는 3보드 방식이며, 입구 제어, 주차칸 감지, 출구 LCD 표시를 분리해서 처리한다. 여기에 Node.js 기반 관리자 페이지를 붙여 실시간 상태와 요금 흐름을 웹에서 확인할 수 있다.

| 구성 | 역할 | 파일 |
| --- | --- | --- |
| Arduino Uno 1 | 입구/출구 조도센서, 입구/출구 차단기 | `sketches/uno_gate/uno_gate.ino` |
| Arduino Uno 2 | 주차칸 2개 감지, LED 상태 표시 | `sketches/uno_slots/uno_slots.ino` |
| Arduino Uno 3 | 출구 LCD 전광판 | `sketches/uno_lcd/uno_lcd.ino` |
| Node.js 서버 | 시리얼 중계, 상태 저장, 요금 계산 | `요금 (NodeJs Server)/fee-server/src/server.js` |
| 관리자 페이지 | 실시간 주차 현황, 예상 요금, 운영 화면 | `관리자 페이지 (NodeJs Server)` |

## 동작 흐름

1. Uno 1이 입구 조도센서로 차량을 감지한다.
2. 빈자리가 있으면 차단기를 열고, 없으면 닫은 상태를 유지한다.
3. Uno 2가 주차칸 2개의 점유 여부를 감지한다.
4. 차량이 들어가면 입차 시각을 서버에 저장한다.
5. 차량이 빠지면 출차 대기 상태로 바꾼다.
6. Uno 1 출구 센서가 차량을 감지하면 출차를 확정한다.
7. Node.js 서버가 요금을 계산하고 Uno 3 출구 LCD에 표시한다.
8. 관리자 페이지에서 주차칸 상태, 경과 시간, 발생 요금, 요금 기준을 실시간으로 확인한다.

## 관리자 페이지

- 경로: `관리자 페이지 (NodeJs Server)`
- 접속: `http://localhost:3000/admin`
- 계정:
  - `admin1 / 1234`
  - `admin2 / 1234`
- 기능:
  - 실시간 연결 상태 표시
  - 다크/라이트 테마 전환
  - 주차칸 상태 테이블형 모니터링
  - 경과 시간 및 발생 요금 실시간 갱신
  - 요금 책정 기준 안내

## LCD 표시

Uno 3는 출구 LCD만 사용한다.

- 기본 화면: `Parking Status` / `Cars: 현재점유/전체`
- 출차 화면: `Slot n Exit` / `Fee: 금액`

입구 LCD 관련 명령은 제거했다.

## 주요 회로도

- [Arduino Uno 1 회로도](docs/circuits/arduino_uno_1_gate.svg)
- [Arduino Uno 2 회로도](docs/circuits/arduino_uno_2_slots.svg)
- [Arduino Uno 3 회로도](docs/circuits/arduino_uno_3_lcd.svg)
- [Node.js Serial Bridge 회로도](docs/circuits/node_serial_bridge.svg)

## 부품

- Arduino Uno 3개
- 브레드보드 2개 이상
- 조도센서 2개
- 초음파센서 2개
- I2C LCD 1개
- 서보모터 2개
- LED 여러 개
- 점퍼선, 저항, USB 케이블

## 업로드와 실행

### 1. Uno 스케치 업로드

- `sketches/uno_gate/uno_gate.ino`
- `sketches/uno_slots/uno_slots.ino`
- `sketches/uno_lcd/uno_lcd.ino`

### 2. Node.js 서버 실행

```powershell
cd "요금 (NodeJs Server)\fee-server"
npm install
npm start
```

기본 시리얼 포트:

- `GATE_SERIAL_PORT=COM3`
- `SLOT_SERIAL_PORT=COM5`
- `LCD_SERIAL_PORT=COM7`
- `SERIAL_BAUD_RATE=9600`

## 테스트 스케치

- `sketches/test/uno1_gate_full_test/uno1_gate_full_test.ino`
- `sketches/test/exit_servo_only_test/exit_servo_only_test.ino`
- `sketches/test/uno2_slots_full_test/uno2_slots_full_test.ino`
- `sketches/test/uno3_exit_lcd_test/uno3_exit_lcd_test.ino`
- `sketches/test/uno3_i2c_lcd_simple_test/uno3_i2c_lcd_simple_test.ino`

## 문서

- [handover.txt](handover.txt)
- [BOARD_UPLOAD_GUIDE.md](BOARD_UPLOAD_GUIDE.md)

## 현재 값

- 입구 서보모터 이동 시간: `100ms`
- 출구 서보모터 이동 시간: `200ms`
- 주차칸 감지 거리: 1번 `16cm`, 2번 `10cm`
- 조도센서 차단 임계값: `200`
- 요금 기준: `30분 1000원`, 이후 `10분당 500원`
