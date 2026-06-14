#include <LiquidCrystal_I2C.h>
#include <Servo.h>

const int ENTRANCE_LIGHT_PIN = A0;      // 입구 차량 감지 조도센서 핀
const int EXIT_LIGHT_PIN = A1;          // 출구 차량 감지 조도센서 핀
const int ENTRANCE_SERVO_PIN = 9;       // 입구 차단기 서보모터 제어 핀
const int EXIT_SERVO_PIN = 8;           // 출구 차단기 서보모터 제어 핀

const int LIGHT_BLOCKED_THRESHOLD = 200;      // 차량 그림자로 판단할 조도 기준
const int SERVO_STOP_ANGLE = 90;              // 연속회전 서보 정지 신호
const int SERVO_OPEN_ROTATE_ANGLE = 180;      // 차단기 열림 방향 회전 신호
const int SERVO_CLOSE_ROTATE_ANGLE = 0;       // 차단기 닫힘 방향 회전 신호
const unsigned long SERVO_ROTATE_TIME_MS = 650; // 차단기 1회 이동 시간
const unsigned long GATE_OPEN_TIME_MS = 3000;   // 차단기 열림 유지 시간
const unsigned long LIGHT_LOG_INTERVAL_MS = 1000; // 조도센서 로그 출력 주기

LiquidCrystal_I2C lcd(0x27, 16, 2); // 주차장 상태 표시 LCD
Servo entranceBarrierServo;         // 입구 차단기 제어 서보 객체
Servo exitBarrierServo;             // 출구 차단기 제어 서보 객체

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

BarrierState entranceBarrier = {&entranceBarrierServo, BARRIER_STOPPED, 0}; // 입구 차단기 동작 상태
BarrierState exitBarrier = {&exitBarrierServo, BARRIER_STOPPED, 0};         // 출구 차단기 동작 상태

int cachedEmptySlots = 2;                  // Node.js 서버에서 마지막으로 받은 빈자리 수
bool previousEntranceDetected = false;     // 입구 감지 중복 처리 방지 상태
bool previousExitDetected = false;         // 출차 감지 중복 처리 방지 상태
unsigned long lastLightLoggedAt = 0;       // 마지막 조도 로그 출력 시각

/**
 * LCD 두 줄에 주차장 상태 메시지를 출력한다.
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
 * 연속회전 서보를 정지 상태로 둔다.
 * @param {Servo&} barrierServo - 제어할 차단기 서보 객체
 * @returns {void} 반환값 없음
 */
void stopGateServo(Servo& barrierServo) {
  barrierServo.write(SERVO_STOP_ANGLE);
}

/**
 * 차단기 열림 방향으로 짧게 회전을 시작한다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startOpeningGate(BarrierState& barrier) {
  barrier.servo->write(SERVO_OPEN_ROTATE_ANGLE);
  barrier.phase = BARRIER_OPENING;
  barrier.changedAt = millis();
}

/**
 * 차단기 닫힘 방향으로 짧게 회전을 시작한다.
 * @param {BarrierState&} barrier - 제어할 차단기 상태
 * @returns {void} 반환값 없음
 */
void startClosingGate(BarrierState& barrier) {
  barrier.servo->write(SERVO_CLOSE_ROTATE_ANGLE);
  barrier.phase = BARRIER_CLOSING;
  barrier.changedAt = millis();
}

/**
 * 연속회전 서보가 계속 돌지 않도록 이동 시간 이후 정지시킨다.
 * @param {BarrierState&} barrier - 갱신할 차단기 상태
 * @returns {void} 반환값 없음
 */
void updateGate(BarrierState& barrier) {
  unsigned long elapsed = millis() - barrier.changedAt;

  if (barrier.phase == BARRIER_OPENING && elapsed >= SERVO_ROTATE_TIME_MS) {
    stopGateServo(*barrier.servo);
    barrier.phase = BARRIER_OPEN;
    barrier.changedAt = millis();
    return;
  }

  if (barrier.phase == BARRIER_OPEN && elapsed >= GATE_OPEN_TIME_MS) {
    startClosingGate(barrier);
    return;
  }

  if (barrier.phase == BARRIER_CLOSING && elapsed >= SERVO_ROTATE_TIME_MS) {
    stopGateServo(*barrier.servo);
    barrier.phase = BARRIER_STOPPED;
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
 * 조도센서 튜닝을 위해 입구와 출구 값을 주기적으로 출력한다.
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
 * Node.js 서버에서 받은 한 줄 명령을 처리한다.
 * @param {String} line - Node.js 서버가 전송한 명령 문자열
 * @returns {void} 반환값 없음
 */
void handleNodeLine(String line) {
  line.trim();

  if (line.startsWith("EMPTY,")) {
    cachedEmptySlots = line.substring(6).toInt();
    return;
  }

  if (line.startsWith("FEE,")) {
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);
    String slotId = line.substring(firstComma + 1, secondComma);
    String fee = line.substring(secondComma + 1);
    showMessage("Slot " + slotId + " Exit", "Fee: " + fee);
  }
}

/**
 * USB Serial로 들어온 Node.js 서버 명령을 읽는다.
 * @returns {void} 반환값 없음
 */
void readNodeMessages() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleNodeLine(line);
  }
}

/**
 * 입구 차량 감지 결과에 따라 차단기와 LCD를 제어한다.
 * @returns {void} 반환값 없음
 */
void handleEntranceVehicle(int entranceLightValue) {
  bool entranceDetected = isVehicleDetectedByLight(entranceLightValue);

  if (entranceDetected && !previousEntranceDetected) {
    if (cachedEmptySlots > 0) {
      showMessage("Entrance Open", "Empty: " + String(cachedEmptySlots));
      if (entranceBarrier.phase == BARRIER_STOPPED) {
        startOpeningGate(entranceBarrier);
      }
    } else {
      showMessage("Parking Full", "Gate Closed");
      stopGateServo(entranceBarrierServo);
      entranceBarrier.phase = BARRIER_STOPPED;
    }
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 출구 차량을 감지하면 Node.js 서버에 출차 확정을 알린다.
 * @param {int} exitLightValue - 출구 조도센서 값
 * @returns {void} 반환값 없음
 */
void handleExitVehicle(int exitLightValue) {
  bool exitDetected = isVehicleDetectedByLight(exitLightValue);

  if (exitDetected && !previousExitDetected) {
    Serial.println("BARRIER_EXIT");
    showMessage("Exit Open", "Calculating...");
    if (exitBarrier.phase == BARRIER_STOPPED) {
      startOpeningGate(exitBarrier);
    }
  }

  previousExitDetected = exitDetected;
}

/**
 * 입구/차단기 제어 보드를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  entranceBarrierServo.attach(ENTRANCE_SERVO_PIN);
  exitBarrierServo.attach(EXIT_SERVO_PIN);
  stopGateServo(entranceBarrierServo);
  stopGateServo(exitBarrierServo);
  showMessage("Smart Parking", "Gates Ready");
}

/**
 * 입구 감지, 차단기 통과 감지, Node.js 서버 메시지를 반복 처리한다.
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
