# 보드별 업로드 안내

이 프로젝트는 여러 보드가 역할을 나눠서 동작하므로 루트 폴더에 통합 실행용 `.ino` 파일을 두지 않습니다.

| 보드 | 업로드할 파일 | 역할 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/uno_gate/uno_gate.ino` | 입구/출구 감지, 입구/출구 차단기 |
| Arduino Uno 2 | `sketches/uno_slots/uno_slots.ino` | 주차칸 2개 감지, 칸별 LED |
| Arduino Uno 3 | `sketches/uno_lcd/uno_lcd.ino` | 입구 LCD, 출구 LCD 전광판 |
| PC Node.js | `요금 (NodeJs Server)/fee-server` | Uno 3개 USB Serial 중계, 요금 계산, LCD 표시 명령 전송 |

Arduino IDE에서 각 스케치 폴더를 열어 해당 보드에 각각 업로드하면 됩니다.

현재 메인 구조는 Arduino Uno 3대와 PC Node.js 서버를 사용합니다.

## 테스트용 스케치

| 보드 | 업로드할 파일 | 테스트 대상 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/test/uno1_gate_full_test/uno1_gate_full_test.ino` | 입구/출구 조도센서, 입구/출구 서보 통합 테스트 |
| Arduino Uno 1 | `sketches/test/exit_servo_only_test/exit_servo_only_test.ino` | 출구 서보 단독 테스트 |
| Arduino Uno 2 | `sketches/test/uno2_slots_full_test/uno2_slots_full_test.ino` | 주차칸 초음파센서 2개와 LED 4개 테스트 |
| Arduino Uno 3 | `sketches/test/uno3_lcd_dual_test/uno3_lcd_dual_test.ino` | 입구/출구 LCD 2개 표시 테스트 |

아직 주차칸 센서를 연결하지 않은 상태에서는 `uno1_gate_full_test.ino`를 먼저 업로드해서 Uno 1에 부착한 부품만 확인하면 됩니다.

테스트 스케치는 조도값을 0.5초마다 출력하고, 출구 감지 순간에는 `BARRIER_EXIT, Exit Light: 값`과 `BARRIER_EXIT`를 함께 출력합니다.
