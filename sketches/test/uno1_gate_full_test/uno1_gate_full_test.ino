#include <LiquidCrystal_I2C.h>
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

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo entranceServo;
Servo exitServo;

enum BarrierPhase {
  BARRIER_STOPPED,
  BARRIER_OPENING,
  BARRIER_OPEN,
  BARRIER_CLOSING
};

struct BarrierState {
  Servo* servo;
  const char* name;
  BarrierPhase phase;
  unsigned long changedAt;
  bool clockwise;
  unsigned long rotateTimeMs;
};

BarrierState entranceBarrier = {&entranceServo, "Entrance", BARRIER_STOPPED, 0, false, ENTRANCE_SERVO_ROTATE_TIME_MS};
BarrierState exitBarrier = {&exitServo, "Exit", BARRIER_STOPPED, 0, true, EXIT_SERVO_ROTATE_TIME_MS};

bool previousEntranceDetected = false;
bool previousExitDetected = false;
unsigned long lastLightLoggedAt = 0;
bool startupServoTestDone = false;

/**
 * LCD에 두 줄 메시지를 표시한다.
 * @param {String} firstLine - 첫 번째 줄 메시지
 * @param {String} secondLine - 두 번째 줄 메시지
 * @returns {void} 반환값 없음
 */
void showMessage(String firstLine, String secondLine) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(firstLine.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(secondLine.substring(0, 16));
}

/**
 * 서보를 정지 상태로 맞춘다.
 * @param {Servo&} targetServo - 제어할 서보
 * @returns {void} 반환값 없음
 */
void stopServo(Servo& targetServo) {
  targetServo.write(SERVO_STOP_ANGLE);
}

/**
 * 차단기를 여는 방향으로 회전시킨다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startOpeningGate(BarrierState& barrier) {
  barrier.servo->write(barrier.clockwise ? EXIT_SERVO_OPEN_ROTATE_ANGLE : ENTRANCE_SERVO_OPEN_ROTATE_ANGLE);
  barrier.phase = BARRIER_OPENING;
  barrier.changedAt = millis();
  Serial.print(barrier.name);
  Serial.println(" gate opening");
}

/**
 * 차단기를 닫는 방향으로 회전시킨다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startClosingGate(BarrierState& barrier) {
  barrier.servo->write(barrier.clockwise ? EXIT_SERVO_CLOSE_ROTATE_ANGLE : ENTRANCE_SERVO_CLOSE_ROTATE_ANGLE);
  barrier.phase = BARRIER_CLOSING;
  barrier.changedAt = millis();
  Serial.print(barrier.name);
  Serial.println(" gate closing");
}

/**
 * 서보가 지정 시간 동안만 동작하도록 차단기 상태를 갱신한다.
 * @param {BarrierState&} barrier - 갱신할 차단기 상태
 * @returns {void} 반환값 없음
 */
void updateGate(BarrierState& barrier) {
  unsigned long elapsed = millis() - barrier.changedAt;

  if (barrier.phase == BARRIER_OPENING && elapsed >= barrier.rotateTimeMs) {
    stopServo(*barrier.servo);
    barrier.phase = BARRIER_OPEN;
    barrier.changedAt = millis();
    return;
  }

  if (barrier.phase == BARRIER_OPEN && elapsed >= GATE_OPEN_TIME_MS) {
    startClosingGate(barrier);
    return;
  }

  if (barrier.phase == BARRIER_CLOSING && elapsed >= barrier.rotateTimeMs) {
    stopServo(*barrier.servo);
    barrier.phase = BARRIER_STOPPED;
    Serial.print(barrier.name);
    Serial.println(" gate stopped");
  }
}

/**
 * 조도값으로 차량 감지 여부를 판단한다.
 * @param {int} lightValue - 조도센서 값
 * @returns {bool} 차량으로 판단되면 true
 */
bool isVehicleDetectedByLight(int lightValue) {
  return lightValue <= LIGHT_BLOCKED_THRESHOLD;
}

/**
 * 조도값을 0.5초 간격으로 출력한다.
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
 * 입구 차단기의 초기 시험 동작을 수행한다.
 * @param {BarrierState&} barrier - 시험할 차단기 상태
 * @returns {void} 반환값 없음
 */
void runBarrierStartupTest(BarrierState& barrier) {
  startOpeningGate(barrier);
  delay(barrier.rotateTimeMs);
  stopServo(*barrier.servo);
  delay(300);

  startClosingGate(barrier);
  delay(barrier.rotateTimeMs);
  stopServo(*barrier.servo);
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
 * 차량 유입을 감지해 입구 차단기를 제어한다.
 * @param {int} entranceLightValue - 입구 조도값
 * @returns {void} 반환값 없음
 */
void handleEntranceTest(int entranceLightValue) {
  bool entranceDetected = isVehicleDetectedByLight(entranceLightValue);

  Serial.print("[entrance] light=");
  Serial.print(entranceLightValue);
  Serial.print(", detected=");
  Serial.print(entranceDetected ? "1" : "0");
  Serial.print(", prev=");
  Serial.println(previousEntranceDetected ? "1" : "0");

  if (entranceDetected && !previousEntranceDetected) {
    Serial.print("ENTRANCE_DETECTED, Entrance Light: ");
    Serial.println(entranceLightValue);
    showMessage("Entrance Test", "Gate Open");

    if (entranceBarrier.phase == BARRIER_STOPPED) {
      startOpeningGate(entranceBarrier);
    }
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 차량 유출을 감지해 출구 차단기를 제어한다.
 * @param {int} exitLightValue - 출구 조도값
 * @returns {void} 반환값 없음
 */
void handleExitTest(int exitLightValue) {
  bool exitDetected = isVehicleDetectedByLight(exitLightValue);

  if (exitDetected && !previousExitDetected) {
    Serial.print("BARRIER_EXIT, Exit Light: ");
    Serial.println(exitLightValue);
    Serial.println("BARRIER_EXIT");
    showMessage("Exit Test", "Gate Open");

    if (exitBarrier.phase == BARRIER_STOPPED) {
      startOpeningGate(exitBarrier);
    }
  }

  previousExitDetected = exitDetected;
}

/**
 * 테스트용 보드 초기화를 수행한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  entranceServo.attach(ENTRANCE_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);
  stopServo(entranceServo);
  stopServo(exitServo);

  delay(300);
  runStartupServoTest();

  Serial.println("=== Uno 1 Gate Full Test Start ===");
  Serial.println("A0: Entrance light, A1: Exit light");
  Serial.println("D9: Entrance servo, D8: Exit servo");
  Serial.println("LCD: I2C 0x27, SDA A4, SCL A5");
  Serial.print("Threshold: ");
  Serial.println(LIGHT_BLOCKED_THRESHOLD);

  showMessage("Uno 1 Test", "Ready");
}

/**
 * 테스트용 센서와 차단기 상태를 반복 갱신한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  int entranceLightValue = analogRead(ENTRANCE_LIGHT_PIN);
  int exitLightValue = analogRead(EXIT_LIGHT_PIN);

  logLightValues(entranceLightValue, exitLightValue);
  handleEntranceTest(entranceLightValue);
  handleExitTest(exitLightValue);
  updateGate(entranceBarrier);
  updateGate(exitBarrier);
  delay(100);
}
