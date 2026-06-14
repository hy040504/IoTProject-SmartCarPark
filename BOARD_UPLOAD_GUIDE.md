# 보드 업로드 가이드

현재 프로젝트 기준으로 어떤 스케치를 어떤 보드에 올려야 하는지 정리한 문서다.

## 1. 기본 업로드 대상

| 보드 | 업로드 파일 | 역할 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/uno_gate/uno_gate.ino` | 입구/출구 조도센서, 차단기 제어 |
| Arduino Uno 2 | `sketches/uno_slots/uno_slots.ino` | 주차칸 2개 감지, LED 제어 |
| Arduino Uno 3 | `sketches/uno_lcd/uno_lcd.ino` | 출구 LCD 표시 |

## 2. 테스트 스케치

| 보드 | 업로드 파일 | 확인 목적 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/test/uno1_gate_full_test/uno1_gate_full_test.ino` | 센서와 차단기 전체 확인 |
| Arduino Uno 1 | `sketches/test/exit_servo_only_test/exit_servo_only_test.ino` | 출구 서보 단독 확인 |
| Arduino Uno 2 | `sketches/test/uno2_slots_full_test/uno2_slots_full_test.ino` | 초음파센서와 LED 확인 |
| Arduino Uno 3 | `sketches/test/uno3_exit_lcd_test/uno3_exit_lcd_test.ino` | 출구 LCD 단독 확인 |

## 3. 업로드 순서

1. Arduino IDE 또는 `arduino-cli`로 각 보드에 스케치를 업로드한다.
2. 업로드 후 시리얼 모니터는 닫는다.
3. USB 포트를 확인한다.
4. Node.js 서버를 실행한다.

## 4. Node.js 서버 실행

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

## 5. 주의할 점

- Node.js 서버와 Arduino IDE 시리얼 모니터는 같은 포트를 동시에 열 수 없다.
- Uno 3는 출구 LCD만 사용한다.
- 입구 LCD 명령은 제거했다.
- 포트 번호는 PC 환경에 따라 달라질 수 있으니 `serial/status`를 확인한 뒤 맞춰야 한다.

## 6. 현재 기준 요약

- Uno 1: 입구/출구 차단기
- Uno 2: 주차칸 상태
- Uno 3: 출구 LCD
- 서버: 상태 저장과 요금 계산
