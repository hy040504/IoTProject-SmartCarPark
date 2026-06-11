# 보드별 업로드 안내

이 프로젝트는 여러 보드가 역할을 나눠서 동작하므로 루트 폴더에 통합 실행용 `.ino` 파일을 두지 않습니다.

| 보드 | 업로드할 파일 | 역할 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/uno_gate/uno_gate.ino` | 입구/출구 감지, LCD, 입구/출구 차단기 |
| Arduino Uno 2 | `sketches/uno_slots/uno_slots.ino` | 주차칸 2개 감지, 칸별 LED |
| Wemos D1 R1 1 | `sketches/wemos_gateway/wemos_gateway.ino` | 요금 서버 통신 게이트웨이 |
| Wemos D1 R1 2 | `sketches/wemos_monitor/wemos_monitor.ino` | 서버 상태 모니터링 |

Arduino IDE에서 각 스케치 폴더를 열어 해당 보드에 각각 업로드하면 됩니다.

## 테스트용 스케치

| 보드 | 업로드할 파일 | 테스트 대상 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/test/uno_gate_test/uno_gate_test.ino` | 입구 조도센서, 출구 조도센서, LCD, 입구 서보, 출구 서보 |
| Arduino Uno 1 | `sketches/test/uno_gate_no_lcd_test/uno_gate_no_lcd_test.ino` | LCD 제외, 조도센서와 서보만 테스트 |
| Arduino Uno 1 | `sketches/test/uno_d8_servo_test/uno_d8_servo_test.ino` | D8 출구 서보 단독 테스트 |
| Arduino Uno 1 | `sketches/test/uno_i2c_scanner/uno_i2c_scanner.ino` | LCD I2C 주소와 배선 확인 |

아직 Wemos나 주차칸 센서를 연결하지 않은 상태에서는 `uno_gate_test.ino`를 먼저 업로드해서 현재 부착한 부품만 확인하면 됩니다.

부팅 메시지만 나오고 조도값 로그가 더 나오지 않으면 LCD 초기화에서 멈췄을 가능성이 큽니다. 이때는 먼저 `uno_gate_no_lcd_test.ino`로 조도센서와 서보를 확인하고, 그 다음 `uno_i2c_scanner.ino`로 LCD 주소가 `0x27`인지 `0x3F`인지 확인합니다.
