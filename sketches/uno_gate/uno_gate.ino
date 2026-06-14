#include <Servo.h>

const int ENTRANCE_LIGHT_PIN = A0;
const int EXIT_LIGHT_PIN = A1;
const int ENTRANCE_SERVO_PIN = 9;
const int EXIT_SERVO_PIN = 8;

const int LIGHT_BLOCKED_THRESHOLD = 200;
const int SERVO_STOP_ANGLE = 90;
const int ENTRANCE_SERVO_OPEN_ROTATE_ANGLE = 180;
const int ENTRANCE_SERVO_CLOSE_ROTATE_ANGLE = 0;
const int EXIT_SERVO_OPEN_ROTATE_ANGLE = 0;
const int EXIT_SERVO_CLOSE_ROTATE_ANGLE = 180;
const unsigned long ENTRANCE_SERVO_ROTATE_TIME_MS = 100;
const unsigned long EXIT_SERVO_ROTATE_TIME_MS = 200;
const unsigned long GATE_OPEN_TIME_MS = 3000;
const unsigned long LIGHT_LOG_INTERVAL_MS = 500;

Servo entranceBarrierServo;
Servo exitBarrierServo;

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
  bool clockwise;
  unsigned long rotateTimeMs;
};

BarrierState entranceBarrier = {&entranceBarrierServo, BARRIER_STOPPED, 0, false, ENTRANCE_SERVO_ROTATE_TIME_MS};
BarrierState exitBarrier = {&exitBarrierServo, BARRIER_STOPPED, 0, true, EXIT_SERVO_ROTATE_TIME_MS};

int cachedEmptySlots = 2;
bool previousEntranceDetected = false;
bool previousExitDetected = false;
unsigned long lastLightLoggedAt = 0;
bool startupServoTestDone = false;

/**
 * 서보를 정지 상태로 맞춘다.
 * @param {Servo&} barrierServo - 제어할 서보
 * @returns {void} 반환값 없음
 */
void stopGateServo(Servo& barrierServo) {
  barrierServo.write(SERVO_STOP_ANGLE);
}

/**
 * 차단기 개방 동작을 시작한다.
 * @param {BarrierState&} barrier - 차단기 상태
 * @returns {void} 반환값 없음
 */
void startOpeningGate(BarrierState& barrier) {
  barrier.servo->write(barrier.clockwise ? EXIT_SERVO_OPEN_ROTATE_ANGLE : ENTRANCE_SERVO_OPEN_ROTATE_ANGLE);
  barrier.phase = BARRIER_OPENING;
  barrier.changedAt = millis();
}

/**
 * 차단기 폐쇄 동작을 시작한다.
 * @param {BarrierState&} barrier - 차단기 상태
 * @returns {void} 반환값 없음
 */
void startClosingGate(BarrierState& barrier) {
  barrier.servo->write(barrier.clockwise ? EXIT_SERVO_CLOSE_ROTATE_ANGLE : ENTRANCE_SERVO_CLOSE_ROTATE_ANGLE);
  barrier.phase = BARRIER_CLOSING;
  barrier.changedAt = millis();
}

/**
 * 서보가 지정 시간 동안만 움직이도록 상태를 갱신한다.
 * @param {BarrierState&} barrier - 차단기 상태
 * @returns {void} 반환값 없음
 */
void updateGate(BarrierState& barrier) {
  unsigned long elapsed = millis() - barrier.changedAt;

  if (barrier.phase == BARRIER_OPENING && elapsed >= barrier.rotateTimeMs) {
    stopGateServo(*barrier.servo);
    barrier.phase = BARRIER_OPEN;
    barrier.changedAt = millis();
    return;
  }

  if (barrier.phase == BARRIER_OPEN && elapsed >= GATE_OPEN_TIME_MS) {
    startClosingGate(barrier);
    return;
  }

  if (barrier.phase == BARRIER_CLOSING && elapsed >= barrier.rotateTimeMs) {
    stopGateServo(*barrier.servo);
    barrier.phase = BARRIER_STOPPED;
  }
}

/**
 * 조도값으로 차량 감지를 판단한다.
 * @param {int} lightValue - 조도센서 값
 * @returns {bool} 차량으로 판단되면 true
 */
bool isVehicleDetectedByLight(int lightValue) {
  return lightValue <= LIGHT_BLOCKED_THRESHOLD;
}

/**
 * 조도값을 0.5초마다 시리얼로 출력한다.
 * @param {int} entranceLightValue - 입구 조도값
 * @param {int} exitLightValue - 출구 조도값
 * @returns {void} 반환값 없음
 */
void logLightValues(int entranceLightValue, int exitLightValue) {
  if (millis() - lastLightLoggedAt < LIGHT_LOG_INTERVAL_MS) {
    return;
  }

  lastLightLoggedAt = millis();
  Serial.print("Entrance Light: ");
  Serial.print(entranceLightValue);
  Serial.print(", Exit Light: ");
  Serial.println(exitLightValue);
}

/**
 * 서버가 보낸 빈자리 수를 반영한다.
 * @param {String} line - 수신한 한 줄
 * @returns {void} 반환값 없음
 */
void handleNodeLine(String line) {
  line.trim();

  if (line.startsWith("EMPTY,")) {
    cachedEmptySlots = line.substring(6).toInt();
  }
}

/**
 * 부팅 직후 차단기 동작 여부를 빠르게 점검한다.
 * @param {BarrierState&} barrier - 점검할 차단기 상태
 * @returns {void} 반환값 없음
 */
void runBarrierStartupTest(BarrierState& barrier) {
  startOpeningGate(barrier);
  delay(barrier.rotateTimeMs);
  stopGateServo(*barrier.servo);
  delay(300);

  startClosingGate(barrier);
  delay(barrier.rotateTimeMs);
  stopGateServo(*barrier.servo);
  barrier.phase = BARRIER_STOPPED;
  barrier.changedAt = millis();
  delay(300);
}

/**
 * 부팅 직후 양쪽 차단기를 한 번씩 시험 동작시킨다.
 * @returns {void} 반환값 없음
 */
void runStartupServoTest() {
  if (startupServoTestDone) {
    return;
  }

  startupServoTestDone = true;
  Serial.println("=== Startup Servo Test Start ===");
  runBarrierStartupTest(entranceBarrier);
  runBarrierStartupTest(exitBarrier);
  Serial.println("=== Startup Servo Test End ===");
}

/**
 * USB Serial에서 서버 메시지를 읽는다.
 * @returns {void} 반환값 없음
 */
void readNodeMessages() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleNodeLine(line);
  }
}

/**
 * 입구 차량 감지에 따라 차단기를 제어한다.
 * @param {int} entranceLightValue - 입구 조도값
 * @returns {void} 반환값 없음
 */
void handleEntranceVehicle(int entranceLightValue) {
  bool entranceDetected = isVehicleDetectedByLight(entranceLightValue);

  Serial.print("[entrance] light=");
  Serial.print(entranceLightValue);
  Serial.print(", detected=");
  Serial.print(entranceDetected ? "1" : "0");
  Serial.print(", prev=");
  Serial.print(previousEntranceDetected ? "1" : "0");
  Serial.print(", empty=");
  Serial.println(cachedEmptySlots);

  if (entranceDetected && !previousEntranceDetected) {
    Serial.print("ENTRANCE_DETECTED, Entrance Light: ");
    Serial.println(entranceLightValue);

    if (cachedEmptySlots > 0) {
      if (entranceBarrier.phase == BARRIER_STOPPED) {
        startOpeningGate(entranceBarrier);
      }
    } else {
      Serial.println("[entrance] parking full");
      stopGateServo(entranceBarrierServo);
      entranceBarrier.phase = BARRIER_STOPPED;
    }
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 출구 차량 감지에 따라 차단기를 제어한다.
 * @param {int} exitLightValue - 출구 조도값
 * @returns {void} 반환값 없음
 */
void handleExitVehicle(int exitLightValue) {
  bool exitDetected = isVehicleDetectedByLight(exitLightValue);

  if (exitDetected && !previousExitDetected) {
    Serial.print("BARRIER_EXIT, Exit Light: ");
    Serial.println(exitLightValue);
    Serial.println("BARRIER_EXIT");

    if (exitBarrier.phase == BARRIER_STOPPED) {
      startOpeningGate(exitBarrier);
    }
  }

  previousExitDetected = exitDetected;
}

/**
 * 입구와 출구 차단기를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  entranceBarrierServo.attach(ENTRANCE_SERVO_PIN);
  exitBarrierServo.attach(EXIT_SERVO_PIN);
  stopGateServo(entranceBarrierServo);
  stopGateServo(exitBarrierServo);

  delay(300);
  runStartupServoTest();
}

/**
 * 입구와 출구 센서 값을 계속 읽는다.
 * @returns {void} 반환값 없음
 */
void loop() {
  int entranceLightValue = analogRead(ENTRANCE_LIGHT_PIN);
  int exitLightValue = analogRead(EXIT_LIGHT_PIN);

  readNodeMessages();
  logLightValues(entranceLightValue, exitLightValue);
  handleEntranceVehicle(entranceLightValue);
  handleExitVehicle(exitLightValue);
  updateGate(entranceBarrier);
  updateGate(exitBarrier);
  delay(100);
}
