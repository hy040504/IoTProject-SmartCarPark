#include <LiquidCrystal_I2C.h>
#include <Servo.h>

const int ENTRANCE_LIGHT_PIN = A0;      // 입구 조도센서 핀
const int EXIT_LIGHT_PIN = A1;          // 출구 조도센서 핀
const int ENTRANCE_SERVO_PIN = 9;       // 입구 차단기 서보 핀
const int EXIT_SERVO_PIN = 8;           // 출구 차단기 서보 핀

const int LIGHT_BLOCKED_THRESHOLD = 200;      // 차량 그림자로 판단할 조도 기준
const int SERVO_STOP_ANGLE = 90;              // 연속회전 서보 정지 신호
const int SERVO_OPEN_ROTATE_ANGLE = 180;      // 차단기 열림 방향 회전 신호
const int SERVO_CLOSE_ROTATE_ANGLE = 0;       // 차단기 닫힘 방향 회전 신호
const unsigned long SERVO_ROTATE_TIME_MS = 550; // 차단기 1회 이동 시간
const unsigned long GATE_OPEN_TIME_MS = 3000;   // 차단기 열림 유지 시간
const unsigned long LIGHT_LOG_INTERVAL_MS = 500; // 조도센서 로그 출력 주기

LiquidCrystal_I2C lcd(0x27, 16, 2); // Uno 1 장치 상태 확인용 LCD
Servo entranceServo;                // 입구 차단기 테스트 서보 객체
Servo exitServo;                    // 출구 차단기 테스트 서보 객체

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
};

BarrierState entranceBarrier = {&entranceServo, "Entrance", BARRIER_STOPPED, 0}; // 입구 차단기 테스트 상태
BarrierState exitBarrier = {&exitServo, "Exit", BARRIER_STOPPED, 0};             // 출구 차단기 테스트 상태

bool previousEntranceDetected = false; // 입구 센서 중복 감지 방지 상태
bool previousExitDetected = false;     // 출구 센서 중복 감지 방지 상태
unsigned long lastLightLoggedAt = 0;   // 마지막 조도 로그 출력 시각

/**
 * LCD 두 줄에 테스트 상태를 출력한다.
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
 * 연속회전 서보를 정지 신호로 고정한다.
 * @param {Servo&} targetServo - 제어할 서보 객체
 * @returns {void} 반환값 없음
 */
void stopServo(Servo& targetServo) {
  targetServo.write(SERVO_STOP_ANGLE);
}

/**
 * 차단기 열림 방향 회전을 시작한다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startOpeningGate(BarrierState& barrier) {
  barrier.servo->write(SERVO_OPEN_ROTATE_ANGLE);
  barrier.phase = BARRIER_OPENING;
  barrier.changedAt = millis();

  Serial.print(barrier.name);
  Serial.println(" gate opening");
}

/**
 * 차단기 닫힘 방향 회전을 시작한다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startClosingGate(BarrierState& barrier) {
  barrier.servo->write(SERVO_CLOSE_ROTATE_ANGLE);
  barrier.phase = BARRIER_CLOSING;
  barrier.changedAt = millis();

  Serial.print(barrier.name);
  Serial.println(" gate closing");
}

/**
 * 연속회전 서보가 설정 시간 이후 멈추도록 차단기 상태를 갱신한다.
 * @param {BarrierState&} barrier - 갱신할 차단기 상태
 * @returns {void} 반환값 없음
 */
void updateGate(BarrierState& barrier) {
  unsigned long elapsed = millis() - barrier.changedAt;

  if (barrier.phase == BARRIER_OPENING && elapsed >= SERVO_ROTATE_TIME_MS) {
    stopServo(*barrier.servo);
    barrier.phase = BARRIER_OPEN;
    barrier.changedAt = millis();
    return;
  }

  if (barrier.phase == BARRIER_OPEN && elapsed >= GATE_OPEN_TIME_MS) {
    startClosingGate(barrier);
    return;
  }

  if (barrier.phase == BARRIER_CLOSING && elapsed >= SERVO_ROTATE_TIME_MS) {
    stopServo(*barrier.servo);
    barrier.phase = BARRIER_STOPPED;
    Serial.print(barrier.name);
    Serial.println(" gate stopped");
  }
}

/**
 * 조도센서 값으로 차량 감지 여부를 판단한다.
 * @param {int} lightValue - 확인할 조도센서 값
 * @returns {bool} 차량이 감지되면 true, 아니면 false
 */
bool isVehicleDetectedByLight(int lightValue) {
  return lightValue <= LIGHT_BLOCKED_THRESHOLD;
}

/**
 * 입구와 출구 조도센서 값을 0.5초마다 출력한다.
 * @param {int} entranceLightValue - 입구 조도센서 값
 * @param {int} exitLightValue - 출구 조도센서 값
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
 * 입구 조도센서 감지 시 입구 차단기와 LCD를 테스트한다.
 * @param {int} entranceLightValue - 입구 조도센서 값
 * @returns {void} 반환값 없음
 */
void handleEntranceTest(int entranceLightValue) {
  bool entranceDetected = isVehicleDetectedByLight(entranceLightValue);

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
 * 출구 조도센서 감지 시 출구 차단기와 Serial 이벤트를 테스트한다.
 * @param {int} exitLightValue - 출구 조도센서 값
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
 * Uno 1에 연결된 LCD, 조도센서, 서보모터 테스트 환경을 초기화한다.
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

  Serial.println("=== Uno 1 Gate Full Test Start ===");
  Serial.println("A0: Entrance light, A1: Exit light");
  Serial.println("D9: Entrance servo, D8: Exit servo");
  Serial.println("LCD: I2C 0x27, SDA A4, SCL A5");
  Serial.print("Threshold: ");
  Serial.println(LIGHT_BLOCKED_THRESHOLD);

  showMessage("Uno 1 Test", "Ready");
}

/**
 * Uno 1 장치들이 단독으로 동작하는지 반복 확인한다.
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
