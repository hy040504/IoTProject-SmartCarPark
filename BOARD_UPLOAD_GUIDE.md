# 보드별 업로드 안내

이 프로젝트는 여러 보드가 역할을 나눠서 동작하므로 루트 폴더에 통합 실행용 `.ino` 파일을 두지 않습니다.

| 보드 | 업로드할 파일 | 역할 |
| --- | --- | --- |
| Arduino Uno 1 | `sketches/uno_gate/uno_gate.ino` | 입구/출구 감지, LCD, 입구/출구 차단기, 만차 경고 |
| Arduino Uno 2 | `sketches/uno_slots/uno_slots.ino` | 주차칸 2개 감지, 칸별 LED |
| Wemos D1 R1 1 | `sketches/wemos_gateway/wemos_gateway.ino` | 요금 서버 통신 게이트웨이 |
| Wemos D1 R1 2 | `sketches/wemos_monitor/wemos_monitor.ino` | 서버 상태 모니터링 |

Arduino IDE에서 각 스케치 폴더를 열어 해당 보드에 각각 업로드하면 됩니다.
