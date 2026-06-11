#include <Servo.h>

const int ENTRANCE_LIGHT_PIN = A0;      // 입구 조도센서 핀
const int EXIT_LIGHT_PIN = A1;          // 출구 조도센서 핀
const int ENTRANCE_SERVO_PIN = 9;       // 입구 차단기 서보 핀
const int EXIT_SERVO_PIN = 8;           // 출구 차단기 서보 핀
const int STATUS_LED_PIN = 13;          // 스케치 실행 확인용 내장 LED 핀

const int LIGHT_BLOCKED_THRESHOLD = 200;      // 차량 감지 조도 기준
const int SERVO_STOP_ANGLE = 90;              // 연속회전 서보 정지 신호
const int SERVO_OPEN_ROTATE_ANGLE = 180;      // 차단기 열림 방향 회전 신호
const int SERVO_CLOSE_ROTATE_ANGLE = 0;       // 차단기 닫힘 방향 회전 신호
const unsigned long SERVO_ROTATE_TIME_MS = 650; // 차단기 1회 이동 시간
const unsigned long GATE_HOLD_TIME_MS = 2000;   // 테스트용 차단기 열림 유지 시간
const unsigned long SERIAL_LOG_MS = 300;        // 시리얼 로그 출력 주기
const unsigned long HEARTBEAT_MS = 500;         // 실행 확인 LED 점멸 주기

enum BarrierPhase {
  BARRIER_STOPPED,
  BARRIER_OPENING,
  BARRIER_OPEN,
  BARRIER_CLOSING
};

struct BarrierState {
  Servo* servo;
  BarrierPhase phase;
  unsigned long changedAt;
};

Servo entranceServo; // 입구 차단기 서보 객체
Servo exitServo;     // 출구 차단기 서보 객체

BarrierState entranceBarrier = {&entranceServo, BARRIER_STOPPED, 0}; // 입구 차단기 동작 상태
BarrierState exitBarrier = {&exitServo, BARRIER_STOPPED, 0};         // 출구 차단기 동작 상태

bool previousEntranceDetected = false; // 입구 감지 중복 동작 방지 상태
bool previousExitDetected = false;     // 출구 감지 중복 동작 방지 상태
bool statusLedOn = false;              // 실행 확인 LED 상태

unsigned long lastSerialLoggedAt = 0; // 마지막 시리얼 로그 출력 시각
unsigned long lastHeartbeatAt = 0;    // 마지막 실행 확인 LED 변경 시각

/**
 * 조도센서 값으로 차량 감지 여부를 판단한다.
 * @param {int} lightValue - 조도센서 값
 * @returns {bool} 기준값 이하이면 true, 아니면 false
 */
bool isVehicleDetected(int lightValue) {
  return lightValue <= LIGHT_BLOCKED_THRESHOLD;
}

/**
 * 연속회전 서보를 정지 상태로 둔다.
 * @param {Servo&} servo - 정지시킬 서보 객체
 * @returns {void} 반환값 없음
 */
void stopServo(Servo& servo) {
  servo.write(SERVO_STOP_ANGLE);
}

/**
 * 차단기 열림 방향으로 짧게 회전을 시작한다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startOpeningBarrier(BarrierState& barrier) {
  barrier.servo->write(SERVO_OPEN_ROTATE_ANGLE);
  barrier.phase = BARRIER_OPENING;
  barrier.changedAt = millis();
}

/**
 * 차단기 닫힘 방향으로 짧게 회전을 시작한다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startClosingBarrier(BarrierState& barrier) {
  barrier.servo->write(SERVO_CLOSE_ROTATE_ANGLE);
  barrier.phase = BARRIER_CLOSING;
  barrier.changedAt = millis();
}

/**
 * 연속회전 서보가 계속 돌지 않도록 이동 시간 이후 정지시킨다.
 * @param {BarrierState&} barrier - 갱신할 차단기 상태
 * @returns {void} 반환값 없음
 */
void updateBarrier(BarrierState& barrier) {
  unsigned long elapsed = millis() - barrier.changedAt;

  if (barrier.phase == BARRIER_OPENING && elapsed >= SERVO_ROTATE_TIME_MS) {
    stopServo(*barrier.servo);
    barrier.phase = BARRIER_OPEN;
    barrier.changedAt = millis();
    return;
  }

  if (barrier.phase == BARRIER_OPEN && elapsed >= GATE_HOLD_TIME_MS) {
    startClosingBarrier(barrier);
    return;
  }

  if (barrier.phase == BARRIER_CLOSING && elapsed >= SERVO_ROTATE_TIME_MS) {
    stopServo(*barrier.servo);
    barrier.phase = BARRIER_STOPPED;
  }
}

/**
 * 스케치 실행 여부를 내장 LED 점멸로 표시한다.
 * @returns {void} 반환값 없음
 */
void updateHeartbeat() {
  if (millis() - lastHeartbeatAt < HEARTBEAT_MS) {
    return;
  }

  lastHeartbeatAt = millis();
  statusLedOn = !statusLedOn;
  digitalWrite(STATUS_LED_PIN, statusLedOn ? HIGH : LOW);
}

/**
 * 조도센서 값을 시리얼 모니터에 출력한다.
 * @param {int} entranceLightValue - 입구 조도센서 값
 * @param {int} exitLightValue - 출구 조도센서 값
 * @returns {void} 반환값 없음
 */
void printLightValues(int entranceLightValue, int exitLightValue) {
  if (millis() - lastSerialLoggedAt < SERIAL_LOG_MS) {
    return;
  }

  lastSerialLoggedAt = millis();
  Serial.print("Entrance(A0): ");
  Serial.print(entranceLightValue);
  Serial.print(", Exit(A1): ");
  Serial.println(exitLightValue);
}

/**
 * 입구 조도센서와 입구 서보 동작을 테스트한다.
 * @param {int} entranceLightValue - 입구 조도센서 값
 * @returns {void} 반환값 없음
 */
void testEntranceBarrier(int entranceLightValue) {
  bool entranceDetected = isVehicleDetected(entranceLightValue);

  if (entranceDetected && !previousEntranceDetected && entranceBarrier.phase == BARRIER_STOPPED) {
    Serial.println("Entrance detected -> D9 move");
    startOpeningBarrier(entranceBarrier);
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 출구 조도센서와 출구 서보 동작을 테스트한다.
 * @param {int} exitLightValue - 출구 조도센서 값
 * @returns {void} 반환값 없음
 */
void testExitBarrier(int exitLightValue) {
  bool exitDetected = isVehicleDetected(exitLightValue);

  if (exitDetected && !previousExitDetected && exitBarrier.phase == BARRIER_STOPPED) {
    Serial.println("Exit detected -> D8 move");
    startOpeningBarrier(exitBarrier);
  }

  previousExitDetected = exitDetected;
}

/**
 * 업로드 직후 D9와 D8 서보 신호가 모두 살아있는지 확인한다.
 * @returns {void} 반환값 없음
 */
void runStartupSelfTest() {
  Serial.println("Self test: D9/D8 open direction");
  entranceServo.write(SERVO_OPEN_ROTATE_ANGLE);
  exitServo.write(SERVO_OPEN_ROTATE_ANGLE);
  delay(SERVO_ROTATE_TIME_MS);
  stopServo(entranceServo);
  stopServo(exitServo);

  delay(500);

  Serial.println("Self test: D9/D8 close direction");
  entranceServo.write(SERVO_CLOSE_ROTATE_ANGLE);
  exitServo.write(SERVO_CLOSE_ROTATE_ANGLE);
  delay(SERVO_ROTATE_TIME_MS);
  stopServo(entranceServo);
  stopServo(exitServo);
}

/**
 * LCD 없이 조도센서와 서보모터만 테스트하도록 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(STATUS_LED_PIN, OUTPUT);
  entranceServo.attach(ENTRANCE_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);
  stopServo(entranceServo);
  stopServo(exitServo);

  Serial.println();
  Serial.println("=== Uno Gate NO LCD Test Start ===");
  Serial.println("Baud: 9600");
  Serial.println("A0: Entrance light, A1: Exit light");
  Serial.println("D9: Entrance servo, D8: Exit servo");
  Serial.println("Threshold: 200");
  Serial.println("Continuous servo: 90 stop, 0/180 rotate");

  runStartupSelfTest();
}

/**
 * LCD 없이 조도센서 값과 서보 동작을 반복 테스트한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  int entranceLightValue = analogRead(ENTRANCE_LIGHT_PIN);
  int exitLightValue = analogRead(EXIT_LIGHT_PIN);

  updateHeartbeat();
  printLightValues(entranceLightValue, exitLightValue);
  testEntranceBarrier(entranceLightValue);
  testExitBarrier(exitLightValue);
  updateBarrier(entranceBarrier);
  updateBarrier(exitBarrier);

  delay(50);
}
