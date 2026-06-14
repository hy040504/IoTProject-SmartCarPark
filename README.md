# 🚗 Smart Parking System

Arduino Uno 3대와 Node.js Serial Bridge를 사용하는 스마트 주차장 프로젝트입니다.  
입구에서는 **조도센서**로 차량 접근을 감지하고, 내부 주차칸은 **초음파센서 2개**로 점유 상태를 확인합니다. 빈자리가 있으면 차단기를 열고, 만차이면 입구 진입을 막습니다. 주차 시간과 요금 계산은 **Node.js Express 요금 서버**가 담당하며, 별도의 **웹 관리자 페이지**에서 실시간 운영 상태를 확인할 수 있습니다.

---

## 🧠 현재 보드 구성

현재 보유한 보드 수를 기준으로 기능을 나눴습니다.

| 구성 요소 | 실행/업로드 대상 | 담당 역할 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/uno_gate/uno_gate.ino` | 입구/출구 조도센서, 입구/출구 차단기 |
| Arduino Uno 2 | `sketches/uno_slots/uno_slots.ino` | 주차칸 2개 초음파센서, 칸별 빨간/초록 LED |
| Arduino Uno 3 | `sketches/uno_lcd/uno_lcd.ino` | 출구 LCD 전광판 |
| PC Node.js 서버 | `요금 (NodeJs Server)/fee-server/src/server.js` | Uno 3개 USB Serial 중계, 슬롯 상태 저장, 요금 계산, LCD 표시 명령 |
| 관리자 페이지 | `관리자 페이지 (NodeJs Server)` | 실시간 주차 현황, 경과 시간, 발생 요금, 요금 기준 표시 |

루트 폴더에는 통합 실행용 `.ino` 파일을 두지 않습니다. 실제 업로드 대상은 `sketches/` 아래의 보드별 `.ino` 파일이며, 루트의 `BOARD_UPLOAD_GUIDE.md`는 어떤 파일을 어느 보드에 업로드해야 하는지 안내합니다.

---

## 🔁 보드 간 통신 구조

```text
Arduino Uno 2: 주차칸 감지
  └─ ENTRY,1 / VACATED,1 같은 문자열 전송
      ↓ USB Serial
Node.js 서버: Serial Bridge + 요금 계산
  ├─ 슬롯별 입차 시간 저장
  ├─ 빈자리 수 계산 후 EMPTY,n 전송
  ├─ 출차 대기 슬롯 저장
  ├─ BARRIER_EXIT 수신 시 요금 계산
  ├─ LCD_STATUS / LCD_EXIT_FEE 표시 명령 전송
  └─ 관리자 페이지용 실시간 상태 전송
      ↓ USB Serial
Arduino Uno 1: 입구/출구 차단기
  ├─ EMPTY,n 수신 후 입차 가능 여부 판단
  ├─ 입구 차량 감지 시 입구 차단기 제어
  └─ 출구 차량 감지 시 BARRIER_EXIT 전송

Arduino Uno 3: LCD 전광판
  └─ 출구 LCD: 기본 주차 현황, 출차 칸/요금 표시
```

핵심은 **Uno는 센서와 장치 제어**, **Node.js는 USB Serial 중계와 상태 저장/요금 계산**, **관리자 페이지는 실시간 모니터링**을 맡는 구조입니다.

---

## ✨ 핵심 기능

| 기능 | 설명 |
| --- | --- |
| 🚦 입구/출구 차량 감지 | 조도센서 2개로 입차 차량과 출차 차량을 각각 감지 |
| 🅿️ 주차칸 상태 확인 | 초음파센서 2개로 1번/2번 주차칸 점유 여부 판단 |
| 📟 출구 LCD 전광판 | Uno 3에서 출차 요금과 현재 주차 현황 표시 |
| 🚧 차단기 제어 | 입구 차단기와 출구 차단기를 각각 서보모터로 제어 |
| 🔴🟢 칸별 LED | 차량 있음: 빨간 LED, 빈자리: 초록 LED |
| 🧾 요금 계산 | 입차/출차 시간을 서버에 기록하고 주차 요금 계산 |
| 🌐 관리자 페이지 | 실시간 연결 상태, 주차칸 상태, 경과 시간, 발생 요금 표시 |
| 🔌 서버 연동 | Node.js가 Uno 3개와 USB Serial로 직접 통신 |

---

## 🧭 전체 동작 흐름

```text
┌──────────────────────┐
│ 1. 입구 조도센서 감지 │
└──────────┬───────────┘
           │
           v
┌────────────────────────────┐
│ 2. 서버가 빈자리 개수 확인  │
└───────┬─────────────┬──────┘
        │             │
        v             v
  빈자리 있음         만차
        │             │
        v             v
┌──────────────┐  ┌────────────────┐
│ 차단기 열림  │  │ 진입 불가 유지  │
│ 차량 입장    │  │ 만차 상태 표시  │
└──────┬───────┘  └────────────────┘
       │
       v
┌────────────────────────────┐
│ 3. 차량이 주차칸에 들어감   │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 4. 입차 시간 서버 등록      │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 5. 주차칸에서 차량 이탈     │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 6. 출차 대기 상태 저장      │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 7. 출구 조도센서 2차 감지   │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 8. 출차 확정 + 요금 계산    │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 9. 출구 LCD / 관리자 페이지 │
│    에 상태와 요금 표시      │
└────────────────────────────┘
```

---

## 🛣️ 시나리오별 동작

### ✅ 입차 가능

```text
입구 차량 감지
→ 빈자리 1개 이상
→ 입구 차단기 열림
→ 차량 진입
```

### ⛔ 만차

```text
입구 차량 감지
→ 주차칸 2개 모두 점유
→ 입구 차단기 닫힘 유지
→ 관리자 페이지와 LCD에 상태 반영
```

### 🧾 입차 등록

```text
차량이 특정 주차칸에 들어옴
→ 해당 칸 초음파센서가 차량 감지
→ 해당 칸 빨간 LED ON
→ Uno 2가 Node.js에 ENTRY,칸번호 전송
→ 서버가 입차 시간 저장
```

### 💳 2단계 출차 정산

```text
1차 감지: 주차칸에서 차량이 빠짐
→ 해당 칸을 출차 대기 상태로 저장
→ 해당 칸 초록 LED ON

2차 감지: 차량이 출구 차단기를 통과함
→ Uno 1이 Node.js에 BARRIER_EXIT 전송
→ Node.js가 출차 대기 슬롯의 주차 시간과 요금 계산
→ Node.js가 Uno 3에 LCD_EXIT_FEE 전송
→ 출구 LCD와 관리자 페이지에 요금 표시
```

출차를 2단계로 나눈 이유는 주차칸 센서의 순간 흔들림을 바로 출차로 처리하지 않기 위해서입니다. 실제 차량이 차단기까지 통과했을 때 출차를 확정합니다.

---

## 🖥️ 관리자 페이지

- 경로: `관리자 페이지 (NodeJs Server)`
- 접속: `http://localhost:3000/admin`
- 계정:
  - `admin1 / 1234`
  - `admin2 / 1234`

### 제공 기능

| 항목 | 설명 |
| --- | --- |
| Online 상태 표시 | 서버 SSE 연결 상태를 실시간 표시 |
| 테마 전환 | Light / Dark 모드 전환 |
| 주차칸 상태 블록 | 테이블형 UI로 주차칸 상태 표시 |
| 주차 기록 블록 | 최근 출차 정산 기록을 세션 메모리에 유지 |
| 진입 시간 / 경과 시간 | 각 차량의 실시간 체류 시간 표시 |
| 발생 요금 | 화면에서 동적으로 증가하는 요금 표시 효과 |
| 요금 기준 안내 | 기본요금과 추가요금 정책 안내 |

---

## 📁 파일 구조

```text
led/
├── BOARD_UPLOAD_GUIDE.md
├── README.md
├── handover.txt
├── genspark_ppt_prompt.txt
├── libraries/
│   └── LiquidCrystal_I2C/
├── docs/
│   └── circuits/
│       ├── arduino_uno_1_gate.svg
│       ├── arduino_uno_2_slots.svg
│       ├── arduino_uno_3_lcd.svg
│       └── node_serial_bridge.svg
├── sketches/
│   ├── uno_gate/
│   │   └── uno_gate.ino
│   ├── uno_slots/
│   │   └── uno_slots.ino
│   ├── uno_lcd/
│   │   ├── parking_lcd_display.h
│   │   └── uno_lcd.ino
│   └── test/
│       ├── exit_servo_only_test/
│       ├── uno1_gate_full_test/
│       ├── uno2_slots_full_test/
│       ├── uno3_exit_lcd_test/
│       └── uno3_i2c_lcd_simple_test/
├── 차단기 (Uno 1)/
├── 주차칸 (Uno 2)/
├── 전광판 (Uno 3, LCD)/
├── 요금 (NodeJs Server)/
│   └── fee-server/
└── 관리자 페이지 (NodeJs Server)/
```

| 파일 | 담당 기능 |
| --- | --- |
| `BOARD_UPLOAD_GUIDE.md` | 보드별 업로드 대상 안내 |
| `sketches/uno_gate/uno_gate.ino` | 입구/출구 조도센서, 입구/출구 차단기 제어 |
| `sketches/uno_slots/uno_slots.ino` | 주차칸 감지와 칸별 LED 제어 |
| `sketches/uno_lcd/uno_lcd.ino` | 출구 LCD 전광판 제어 |
| `관리자 페이지 (NodeJs Server)` | 관리자 로그인/대시보드 UI |
| `docs/circuits/*.svg` | 보드별 회로도 |
| `요금 (NodeJs Server)/fee-server/src/server.js` | Express 서버, Serial Bridge, 상태 저장 |
| `요금 (NodeJs Server)/fee-server/src/feeCalculator.js` | 주차 요금 계산 로직 |

---

## 🔌 기본 핀 설정

> 실제 배선에 따라 스케치 상단 상수 값을 수정하면 됩니다.

| 장치 | 핀 | 위치 |
| --- | --- | --- |
| 입구 조도센서 | `A0` | `sketches/uno_gate/uno_gate.ino` |
| 출구 조도센서 | `A1` | `sketches/uno_gate/uno_gate.ino` |
| 입구 차단기 서보모터 | `D9` | `sketches/uno_gate/uno_gate.ino` |
| 출구 차단기 서보모터 | `D8` | `sketches/uno_gate/uno_gate.ino` |
| 1번 칸 초음파 TRIG | `D4` | `sketches/uno_slots/uno_slots.ino` |
| 1번 칸 초음파 ECHO | `D5` | `sketches/uno_slots/uno_slots.ino` |
| 2번 칸 초음파 TRIG | `D13` | `sketches/uno_slots/uno_slots.ino` |
| 2번 칸 초음파 ECHO | `D12` | `sketches/uno_slots/uno_slots.ino` |
| 1번 칸 빨간 LED | `D8` | `sketches/uno_slots/uno_slots.ino` |
| 1번 칸 초록 LED | `D9` | `sketches/uno_slots/uno_slots.ino` |
| 2번 칸 빨간 LED | `D10` | `sketches/uno_slots/uno_slots.ino` |
| 2번 칸 초록 LED | `D11` | `sketches/uno_slots/uno_slots.ino` |
| 출구 I2C LCD | `A4(SDA)`, `A5(SCL)` | `sketches/uno_lcd/uno_lcd.ino` |

---

## 🖼️ 회로도

- [Arduino Uno 1 회로도](docs/circuits/arduino_uno_1_gate.svg)
- [Arduino Uno 2 회로도](docs/circuits/arduino_uno_2_slots.svg)
- [Arduino Uno 3 회로도](docs/circuits/arduino_uno_3_lcd.svg)
- [Node.js Serial Bridge 회로도](docs/circuits/node_serial_bridge.svg)

현재 시연용 메인 흐름은 **Arduino Uno 3대 + PC Node.js 서버 + 웹 관리자 페이지**를 사용합니다.

---

## 🧰 사용 재료

### 필수 부품

| 분류 | 부품 | 개수 | 용도 |
| --- | --- | ---: | --- |
| 보드 | Arduino Uno | 3개 | 차단기 제어, 주차칸 감지, LCD 전광판 |
| 서버 | Node.js 실행 PC | 1대 | USB Serial 중계와 요금 계산 서버 실행 |
| 회로 구성 | 브레드보드 | 2개 이상 | 센서와 LED 회로 구성 |
| 배선 | 점퍼 와이어 | 충분히 | 보드, 센서, LED, LCD, 서보 연결 |
| 입출구 감지 | 조도센서 | 2개 | 입구 차량과 출구 차량 감지 |
| 입출구 감지 | 조도센서용 저항 | 2개 | 조도센서 분압 회로 구성 |
| 주차칸 감지 | HC-SR04 초음파센서 | 2개 | 1번/2번 주차칸 차량 점유 확인 |
| 표시 장치 | I2C LCD 16x2 | 1개 | 출구 요금 및 주차 현황 표시 |
| 차단기 | 서보모터 | 2개 | 입구/출구 차단기 열림/닫힘 제어 |
| 주차칸 LED | 빨간색 LED | 2개 | 각 주차칸 차량 점유 표시 |
| 주차칸 LED | 초록색 LED | 2개 | 각 주차칸 빈자리 표시 |
| LED 보호 | LED용 저항 | 4개 이상 | LED 전류 제한 |

### 선택 부품

| 부품 | 개수 | 용도 |
| --- | ---: | --- |
| 외부 5V 전원 | 1개 | 서보모터 전류 부족 시 사용 |
| 차량 모형 | 1~2개 | 센서 테스트 및 시연 |
| 주차장 모형 재료 | 필요량 | 발표용 구조 제작 |

---

## ⚙️ 주요 설정값

### 입구/출구 조도센서 기준

```cpp
const int LIGHT_BLOCKED_THRESHOLD = 200;
```

평상시 조도 값이 `400~500`대이고 차량이 센서를 가리면 값이 내려가는 상황을 기준으로 잡았습니다. 현재는 조도 값이 `200` 이하이면 차량이 감지된 것으로 판단합니다.

### 주차칸 차량 감지 거리

```cpp
const int SLOT_OCCUPIED_DISTANCE_CM[SLOT_COUNT] = {16, 10};
```

1번 칸 초음파센서는 `16cm` 이하, 2번 칸 초음파센서는 `10cm` 이하를 감지하면 해당 주차칸을 점유 상태로 봅니다.

### 차단기 서보 동작 시간

```cpp
const unsigned long ENTRANCE_SERVO_ROTATE_TIME_MS = 100;
const unsigned long EXIT_SERVO_ROTATE_TIME_MS = 200;
```

현재는 연속회전형 서보 기준으로 입구 `100ms`, 출구 `200ms` 회전 시간을 사용합니다.

### Serial Bridge 포트

```bash
set GATE_SERIAL_PORT=COM3
set SLOT_SERIAL_PORT=COM5
set LCD_SERIAL_PORT=COM7
set SERIAL_BAUD_RATE=9600
```

Node.js 서버가 Uno 1, Uno 2, Uno 3 USB 포트를 직접 엽니다. 포트 번호는 PC 환경에 맞게 바꿔야 합니다.

---

## 💰 요금 정책

| 구간 | 요금 |
| --- | --- |
| 기본 30분 | `1000원` |
| 이후 10분마다 | `500원` 추가 |

예시:

| 주차 시간 | 계산 요금 |
| --- | --- |
| 1분 | `1000원` |
| 30분 | `1000원` |
| 31분 | `1500원` |
| 45분 | `2000원` |

관리자 페이지의 `발생 요금`은 실시간 모니터링용 시각 효과가 포함되며, 실제 정산 기준은 서버의 `feeCalculator.js`를 따릅니다.

---

## ▶️ 실행 방법

### 1. 요금 서버 실행

```powershell
cd "요금 (NodeJs Server)\fee-server"
npm install
npm start
```

서버 기본 주소:

```text
http://localhost:3000
```

관리자 페이지:

```text
http://localhost:3000/admin
```

Tailscale 관리자 페이지 URL:

```text
start_fee_server.bat 실행 후
tailscale_admin_url.txt 파일에 현재 접속 URL이 저장됨
```

Tailscale URL 파일:

```text
tailscale_admin_url.txt
```

서버 실행 전 포트를 지정하려면 PowerShell에서 아래처럼 실행합니다.

```powershell
$env:GATE_SERIAL_PORT='COM3'
$env:SLOT_SERIAL_PORT='COM5'
$env:LCD_SERIAL_PORT='COM7'
npm start
```

Node.js 서버가 COM 포트를 열고 있는 동안에는 Arduino IDE 업로드가 실패할 수 있습니다. 업로드할 때는 서버를 먼저 종료합니다.

### 1-1. 통합 실행 배치 파일

```text
start_fee_server.bat
```

이 배치 파일은 아래 순서로 자동 실행합니다.

1. Uno 1/2/3 스케치 컴파일
2. Uno 1/2/3 업로드
3. Node.js 서버 실행
4. Tailscale IP 확인
5. 접속 URL을 `tailscale_admin_url.txt`에 저장하고 해당 주소로 브라우저 실행

### 2. Arduino 업로드

보드별로 아래 스케치를 각각 업로드합니다.

| 보드 | Arduino IDE 보드 선택 | 업로드 파일 |
| --- | --- | --- |
| Arduino Uno 1 | Arduino Uno | `sketches/uno_gate/uno_gate.ino` |
| Arduino Uno 2 | Arduino Uno | `sketches/uno_slots/uno_slots.ino` |
| Arduino Uno 3 | Arduino Uno | `sketches/uno_lcd/uno_lcd.ino` |

---

## 🧪 테스트 스케치

- `sketches/test/uno1_gate_full_test/uno1_gate_full_test.ino`
- `sketches/test/exit_servo_only_test/exit_servo_only_test.ino`
- `sketches/test/uno2_slots_full_test/uno2_slots_full_test.ino`
- `sketches/test/uno3_exit_lcd_test/uno3_exit_lcd_test.ino`
- `sketches/test/uno3_i2c_lcd_simple_test/uno3_i2c_lcd_simple_test.ino`

---

## 📟 LCD 표시 상태

| 상황 | LCD 1행 | LCD 2행 |
| --- | --- | --- |
| 기본 상태 | `Parking Status` | `Cars: 현재점유/전체` |
| 만차 경고 | `Parking Full` | `Cars: 2/2` |
| 출차 요금 표시 | `Slot n Exit` | `Fee: 요금` |

---

## ⚠️ 하드웨어 주의사항

- Node.js 서버가 Uno의 COM 포트를 열고 있으면 Arduino IDE 시리얼 모니터나 업로드가 같은 포트를 사용할 수 없습니다.
- 업로드할 때는 Node.js 서버를 종료하고, 업로드 후 다시 서버를 실행합니다.
- 출구 LCD는 I2C 방식이며 `A4(SDA)`, `A5(SCL)`를 사용합니다.
- 입구와 출구는 조도센서를 각각 1개씩 사용합니다.
- 현재 폴더명은 가독성을 위해 `차단기 (Uno 1)`, `주차칸 (Uno 2)`, `요금 (NodeJs Server)`처럼 정리되어 있습니다. 공백과 괄호가 있으므로 터미널에서는 따옴표로 감싸는 편이 안전합니다.

---

## ✅ 구현 현황

- ✅ 입구 조도센서 감지 구조
- ✅ 주차칸 2개 초음파 감지 구조
- ✅ 칸별 빨간/초록 LED 표시
- ✅ 출구 LCD 상태 및 요금 표시
- ✅ 서보모터 차단기 제어
- ✅ 입차 서버 등록
- ✅ Node.js USB Serial Bridge
- ✅ 2단계 출차 확정 로직
- ✅ 웹 관리자 페이지
- ✅ 다크/라이트 테마 전환

---

## 🧾 문서

- [handover.txt](handover.txt)
- [BOARD_UPLOAD_GUIDE.md](BOARD_UPLOAD_GUIDE.md)

---

## 🧪 검증 상태

- Node.js 서버 문법 확인 완료
- 관리자 페이지 `app.js` 문법 확인 완료
- `sketches/uno_gate` Arduino Uno 컴파일 완료
- `sketches/uno_slots` Arduino Uno 컴파일 완료
- `sketches/uno_lcd` Arduino Uno 컴파일 완료
- 테스트 스케치 컴파일 완료
