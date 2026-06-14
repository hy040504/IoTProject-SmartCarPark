#include <Servo.h>

const int EXIT_SERVO_PIN = 8;                 // 출구 서보 신호선 확인 핀
const int SERVO_STOP_ANGLE = 90;              // 연속회전 서보 정지 신호
const int SERVO_CLOCKWISE_ANGLE = 0;          // 시계방향 회전 신호
const int SERVO_COUNTERCLOCKWISE_ANGLE = 180; // 반시계방향 회전 신호
const unsigned long SERVO_MOVE_TIME_MS = 250; // 방향별 회전 유지 시간
const unsigned long SERVO_PAUSE_TIME_MS = 1500; // 동작 사이 대기 시간

Servo exitServo;

/**
 * 지정한 각도 신호를 일정 시간 동안 출력하고 정지한다.
 * @param {int} angle - 서보에 전달할 각도 신호
 * @param {const char*} label - 시리얼 모니터에 출력할 동작 이름
 * @returns {void} 반환값 없음
 */
void runServoStep(int angle, const char* label) {
  Serial.print("Exit servo ");
  Serial.println(label);
  exitServo.write(angle);
  delay(SERVO_MOVE_TIME_MS);

  Serial.println("Exit servo stop");
  exitServo.write(SERVO_STOP_ANGLE);
  delay(SERVO_PAUSE_TIME_MS);
}

/**
 * 출구 서보 단독 테스트 환경을 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  exitServo.attach(EXIT_SERVO_PIN);
  exitServo.write(SERVO_STOP_ANGLE);

  Serial.println("=== Exit Servo Only Test Start ===");
  Serial.println("Signal pin: D8");
  Serial.println("Clockwise: 0, Stop: 90, Counterclockwise: 180");
  delay(SERVO_PAUSE_TIME_MS);
}

/**
 * 출구 서보를 양방향으로 반복 회전시켜 배선과 모터 상태를 확인한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  runServoStep(SERVO_CLOCKWISE_ANGLE, "clockwise");
  runServoStep(SERVO_COUNTERCLOCKWISE_ANGLE, "counterclockwise");
}
