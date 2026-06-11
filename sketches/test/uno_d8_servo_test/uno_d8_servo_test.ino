#include <Servo.h>

const int TEST_SERVO_PIN = 8;                 // 테스트할 서보 신호 핀
const int STATUS_LED_PIN = 13;                // 스케치 실행 확인용 내장 LED 핀
const int SERVO_STOP_ANGLE = 90;              // 연속회전 서보 정지 신호
const int SERVO_FORWARD_ANGLE = 180;          // 한 방향 회전 신호
const int SERVO_REVERSE_ANGLE = 0;            // 반대 방향 회전 신호
const unsigned long SERVO_RUN_TIME_MS = 700;  // 회전 확인 시간
const unsigned long SERVO_WAIT_TIME_MS = 1500; // 다음 테스트 전 대기 시간

Servo testServo; // D8 테스트용 서보 객체
bool statusLedOn = false; // 실행 확인 LED 상태

/**
 * 연속회전 서보를 정지 상태로 둔다.
 * @returns {void} 반환값 없음
 */
void stopServo() {
  testServo.write(SERVO_STOP_ANGLE);
}

/**
 * 지정한 방향으로 짧게 회전한 뒤 정지한다.
 * @param {int} rotateAngle - 서보에 보낼 회전 신호 각도
 * @param {String} label - 시리얼 모니터에 출력할 테스트 이름
 * @returns {void} 반환값 없음
 */
void runServoPulse(int rotateAngle, String label) {
  Serial.println(label);
  testServo.write(rotateAngle);
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(SERVO_RUN_TIME_MS);
  stopServo();
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(SERVO_WAIT_TIME_MS);
}

/**
 * D8에 연결된 서보만 테스트하도록 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(STATUS_LED_PIN, OUTPUT);
  testServo.attach(TEST_SERVO_PIN);
  stopServo();

  Serial.println();
  Serial.println("=== D8 Servo Only Test ===");
  Serial.println("Signal: D8");
  Serial.println("Continuous servo: 90 stop, 0/180 rotate");
}

/**
 * D8 서보를 양방향으로 반복 테스트한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  runServoPulse(SERVO_FORWARD_ANGLE, "D8 -> 180 rotate");
  runServoPulse(SERVO_REVERSE_ANGLE, "D8 -> 0 rotate");
}
