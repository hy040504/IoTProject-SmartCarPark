# 🚗 Smart Parking System

Wemos D1 R1 기반의 스마트 주차장 프로젝트입니다.  
입구에서는 **조도센서**로 차량 접근을 감지하고, 내부 주차칸은 **초음파센서 2개**로 점유 상태를 확인합니다. 빈자리가 있으면 차단기를 열고, 만차이면 LCD와 경고 장치로 진입을 막습니다. 주차 시간과 요금 계산은 별도의 **Node.js Express 요금 서버**가 담당합니다.

---

## ✨ 핵심 기능

| 기능 | 설명 |
| --- | --- |
| 🚦 입구 차량 감지 | 조도센서 값 변화로 입구 차량 대기 상태 확인 |
| 🅿️ 주차칸 상태 확인 | 초음파센서 2개로 1번/2번 주차칸 점유 여부 판단 |
| 📟 LCD 안내 | `Gate Open`, `Parking Full`, 출차 요금 표시 |
| 🚧 차단기 제어 | 빈자리 있음: 서보모터 90도, 만차/대기: 0도 |
| 🔴🟢 칸별 LED | 차량 있음: 빨간 LED, 빈자리: 초록 LED |
| 🧾 요금 계산 | 입차/출차 시간을 서버에 기록하고 주차 요금 계산 |
| 🌐 서버 연동 | Wemos가 Express 서버에 GET 요청 전송 |

---

## 🧭 전체 동작 흐름

```text
┌──────────────────────┐
│ 1. 입구 조도센서 감지 │
└──────────┬───────────┘
           │
           v
┌────────────────────────────┐
│ 2. 주차칸 2개 초음파 확인   │
└──────────┬─────────────────┘
           │
           v
┌────────────────────────────┐
│ 3. 빈자리 개수 계산         │
└───────┬─────────────┬──────┘
        │             │
        v             v
  빈자리 있음       만차
        │             │
        v             v
┌──────────────┐  ┌────────────────┐
│ LCD Gate Open│  │ LCD Parking Full│
│ 차단기 열림  │  │ LED/부저 경고   │
└──────┬───────┘  │ 차단기 닫힘     │
       │          └────────────────┘
       v
┌────────────────────────────┐
│ 4. 차량이 주차칸에 들어감   │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 5. 입차 시간 서버 등록      │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 6. 주차칸에서 차량 이탈     │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 7. 출차 대기 상태 저장      │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 8. 차단기 통과 2차 감지     │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 9. 출차 요청 + 요금 계산    │
└──────────┬─────────────────┘
           v
┌────────────────────────────┐
│ 10. LCD에 요금 표시         │
└────────────────────────────┘
```

---

## 🛣️ 시나리오별 동작

### ✅ 입차 가능

```text
입구 차량 감지
→ 주차칸 2개 상태 확인
→ 빈자리 1개 이상
→ LCD: Gate Open
→ 만차 경고 OFF
→ 서보모터 90도 회전
→ 차단기 열림
```

### ⛔ 만차

```text
입구 차량 감지
→ 주차칸 2개 모두 점유
→ LCD: Parking Full
→ 빨간 LED 또는 부저 경고 ON
→ 서보모터 0도 유지
→ 차단기 닫힘
```

### 🧾 입차 등록

```text
차량이 특정 주차칸에 들어옴
→ 해당 칸 초음파센서가 차량 감지
→ 해당 칸 빨간 LED ON
→ 요금 서버에 GET /parking/entry?slot=칸번호 요청
→ 서버가 입차 시간 저장
```

### 💳 2단계 출차 정산

```text
1차 감지: 주차칸에서 차량이 빠짐
→ 해당 칸을 출차 대기 상태로 저장
→ 해당 칸 초록 LED ON

2차 감지: 차량이 차단기를 통과함
→ 요금 서버에 GET /parking/exit?slot=칸번호 요청
→ 서버가 주차 시간과 요금 계산
→ Wemos가 response에서 fee 값 추출
→ LCD에 요금 표시
```

출차를 2단계로 나눈 이유는 주차칸에서 잠깐 센서가 흔들리는 상황을 바로 출차로 처리하지 않기 위해서입니다. 실제 차량이 차단기까지 통과했을 때 출차를 확정합니다.

---

## 📁 파일 구조

```text
led/
├── led.ino
├── barrier/
│   ├── barrier.h
│   └── entrance_sensor.h
├── parking/
│   ├── parking_slots.h
│   ├── parking_display.h
│   └── parking_alert.h
├── fee/
│   ├── fee_request.h
│   └── fee-server/
│       ├── package.json
│       ├── package-lock.json
│       ├── .gitignore
│       └── src/
│           ├── feeCalculator.js
│           └── server.js
└── README.md
```

| 파일 | 담당 기능 |
| --- | --- |
| `led.ino` | 전체 시스템 흐름 제어 |
| `barrier/entrance_sensor.h` | 입구 조도센서 감지, 차단기 통과 감지 함수 제공 |
| `barrier/barrier.h` | 서보모터 기반 차단기 열림/닫힘 |
| `parking/parking_slots.h` | 주차칸 2개 상태 확인, 칸별 LED 제어, 입차/출차 대기 처리 |
| `parking/parking_display.h` | I2C LCD 메시지 출력 |
| `parking/parking_alert.h` | 만차 경고 LED/부저 제어 |
| `fee/fee_request.h` | Wemos에서 요금 서버로 HTTP GET 요청 전송 |
| `fee/fee-server/src/server.js` | Express API 서버 |
| `fee/fee-server/src/feeCalculator.js` | 주차 요금 계산 로직 |

---

## 🔌 기본 핀 설정

> 실제 배선에 따라 헤더 파일 상단의 상수 값을 수정하면 됩니다.

| 장치 | 핀 | 위치 |
| --- | --- | --- |
| 입구 조도센서 | `A0` | `barrier/entrance_sensor.h` |
| 차단기 서보모터 | `D5` | `barrier/barrier.h` |
| 만차 경고 LED | `D0` | `parking/parking_alert.h` |
| 만차 경고 부저 | `D8` | `parking/parking_alert.h` |
| 1번 칸 초음파 TRIG | `D6` | `parking/parking_slots.h` |
| 1번 칸 초음파 ECHO | `D3` | `parking/parking_slots.h` |
| 2번 칸 초음파 TRIG | `D7` | `parking/parking_slots.h` |
| 2번 칸 초음파 ECHO | `D4` | `parking/parking_slots.h` |
| 1번 칸 빨간 LED | `D9` | `parking/parking_slots.h` |
| 1번 칸 초록 LED | `D10` | `parking/parking_slots.h` |
| 2번 칸 빨간 LED | `D11` | `parking/parking_slots.h` |
| 2번 칸 초록 LED | `D12` | `parking/parking_slots.h` |
| I2C LCD | SDA / SCL | `parking/parking_display.h` |

---

## 🧰 사용 재료

### 필수 부품

| 분류 | 부품 | 개수 | 용도 |
| --- | --- | ---: | --- |
| 보드 | Wemos D1 R1 | 1개 | 센서 입력, LED/LCD/서보 제어, 요금 서버 통신 |
| 회로 구성 | 브레드보드 | 1개 이상 | 센서와 LED 회로 구성 |
| 배선 | 점퍼 와이어 | 충분히 | 보드, 센서, LED, LCD, 부저 연결 |
| 입구 감지 | 조도센서 | 1개 | 입구 차량 접근 감지 |
| 입구 감지 | 조도센서용 저항 | 1개 | 조도센서 분압 회로 구성 |
| 주차칸 감지 | HC-SR04 초음파센서 | 2개 | 1번/2번 주차칸 차량 점유 확인 |
| 표시 장치 | I2C LCD 16x2 | 1개 | Gate Open, Parking Full, 요금 표시 |
| 차단기 | 서보모터 | 1개 | 차단기 열림/닫힘 제어 |
| 차단기 | 차단기 막대 재료 | 1개 | 실제 차단기 팔 역할 |
| 주차칸 LED | 빨간색 LED | 2개 | 각 주차칸 차량 점유 표시 |
| 주차칸 LED | 초록색 LED | 2개 | 각 주차칸 빈자리 표시 |
| 만차 경고 | 빨간색 LED | 1개 | 만차 경고 표시 |
| 만차 경고 | 부저 | 1개 | 만차 경고음 출력 |
| LED 보호 | LED용 저항 | 5개 이상 | LED 전류 제한 |
| 전압 보호 | 전압 분배용 저항 | 4개 이상 | HC-SR04 ECHO 5V 신호를 ESP8266 입력에 맞춤 |
| 서버 | Node.js 실행 PC | 1대 | Express 요금 계산 서버 실행 |

### 선택 부품

| 부품 | 개수 | 용도 |
| --- | ---: | --- |
| 추가 조도센서 또는 적외선 센서 | 1개 | 차단기 통과 전용 감지 센서로 사용 |
| 외부 5V 전원 | 1개 | 서보모터와 센서 전류가 부족할 때 사용 |
| 주차장 모형 재료 | 필요량 | 발표용 주차장 구조 제작 |
| 차량 모형 | 1~2개 | 센서 테스트 및 시연 |

> 현재 코드는 차단기 통과 감지를 입구 조도센서 값으로 재사용합니다. 더 안정적인 출차 정산을 원하면 차단기 전용 센서를 추가하는 구성이 좋습니다.

---

## ⚙️ 주요 설정값

### 입구 조도센서 기준

```cpp
const int ENTRANCE_BLOCKED_THRESHOLD = 450;
```

조도 값이 이 기준 이하이면 차량이 입구에 있다고 판단합니다.

### 주차칸 차량 감지 거리

```cpp
const int SLOT_OCCUPIED_DISTANCE_CM = 5;
```

초음파센서가 `5cm` 이하를 감지하면 해당 주차칸을 점유 상태로 봅니다.

### 차단기 각도

```cpp
const int BARRIER_CLOSED_ANGLE = 0;
const int BARRIER_OPEN_ANGLE = 90;
```

빈자리가 있으면 `90도`, 만차 또는 대기 상태에서는 `0도`로 제어합니다.

### 요금 서버 주소

```cpp
const char FEE_SERVER_BASE_URL[] = "http://192.168.0.10:3000";
```

Wemos에서 접근할 수 있도록 서버 PC의 내부 IP 주소로 바꿔야 합니다.

---

## 🌐 요금 서버 API

| Method | URL | 설명 |
| --- | --- | --- |
| GET | `/parking/entry?slot=1` | 1번 칸 입차 시간 저장 |
| GET | `/parking/entry?slot=2` | 2번 칸 입차 시간 저장 |
| GET | `/parking/exit?slot=1` | 1번 칸 출차 요금 계산 |
| GET | `/parking/exit?slot=2` | 2번 칸 출차 요금 계산 |
| GET | `/parking/sessions` | 현재 주차 중인 차량 목록 확인 |

출차 응답 예시:

```json
{
  "ok": true,
  "receipt": {
    "slotId": "1",
    "enteredAt": "2026-06-11T00:51:57.599Z",
    "exitedAt": "2026-06-11T01:25:57.686Z",
    "parkedMinutes": 34,
    "fee": 1500
  }
}
```

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

---

## ▶️ 실행 방법

### 1. 요금 서버 실행

```bash
cd fee/fee-server
npm install
npm start
```

서버 기본 주소:

```text
http://localhost:3000
```

Wemos 코드에서는 `localhost`가 아니라 서버 PC의 내부 IP를 사용해야 합니다.

### 2. Wemos 설정

`fee/fee_request.h`에서 Wi-Fi와 서버 주소를 수정합니다.

```cpp
const char WIFI_SSID[] = "YOUR_WIFI_SSID";
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
const char FEE_SERVER_BASE_URL[] = "http://192.168.0.10:3000";
```

### 3. Arduino 업로드

Arduino IDE에서 Wemos D1 R1 보드를 선택한 뒤 `led.ino`를 업로드합니다.

---

## 📟 LCD 표시 상태

| 상황 | LCD 1행 | LCD 2행 |
| --- | --- | --- |
| 입차 가능 | `Gate Open` | `Empty: 빈자리수` |
| 만차 | `Parking Full` | `Gate Closed` |
| 출차 요금 표시 | `Slot n Exit` | `Fee: 요금` |

---

## ⚠️ 하드웨어 주의사항

- Wemos D1 R1의 실제 사용 가능 핀은 보드 패키지와 보드 종류에 따라 다를 수 있습니다.
- `D9`, `D10`, `D11`, `D12`는 환경에 따라 바로 인식되지 않을 수 있어 실제 보드 핀맵 확인이 필요합니다.
- HC-SR04의 ECHO는 보통 5V로 출력되므로 ESP8266 입력에는 전압 분배 회로를 사용하는 것이 안전합니다.
- I2C LCD 주소가 `0x27`이 아니면 `parking/parking_display.h`의 `LCD_ADDRESS` 값을 바꿔야 합니다.
- 현재 차단기 통과 감지는 입구 조도센서 값을 재사용합니다. 전용 출차 센서를 추가하면 `isBarrierVehicleDetected()` 함수 내부만 바꾸면 됩니다.

---

## ✅ 구현 현황

- ✅ 입구 조도센서 감지 구조
- ✅ 주차칸 2개 초음파 감지 구조
- ✅ 칸별 빨간/초록 LED 표시
- ✅ LCD 상태 표시
- ✅ 서보모터 차단기 제어
- ✅ 만차 경고 LED/부저 제어
- ✅ 입차 서버 등록
- ✅ 2단계 출차 확정 로직
- ✅ 출차 요금 계산 및 LCD 표시
- ✅ Node.js Express 요금 서버

---

## 🧪 검증 상태

- Node.js 서버 문법 확인 완료
- Express 서버 GET 요청 테스트 완료
- Arduino CLI `1.5.1` 기준 Wemos D1 R1 보드(`esp8266:esp8266:d1`) 컴파일 완료
