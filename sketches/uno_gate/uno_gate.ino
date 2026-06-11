#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

const int WEMOS_RX_PIN = 2;             // Wemos 송신선을 받는 핀
const int WEMOS_TX_PIN = 3;             // Wemos 수신선으로 보내는 핀
const int ENTRANCE_LIGHT_PIN = A0;      // 입구 차량 감지 조도센서 핀
const int EXIT_LIGHT_PIN = A1;          // 출구 차량 감지 조도센서 핀
const int ENTRANCE_SERVO_PIN = 9;       // 입구 차단기 서보모터 제어 핀
const int EXIT_SERVO_PIN = 10;          // 출구 차단기 서보모터 제어 핀
const int FULL_ALERT_LED_PIN = 12;      // 만차 경고 LED 핀
const int FULL_ALERT_BUZZER_PIN = 11;   // 만차 경고 부저 핀

const int LIGHT_BLOCKED_THRESHOLD = 350;      // 차량 그림자로 판단할 조도 기준
const int BARRIER_CLOSED_ANGLE = 0;           // 차단기 닫힘 각도
const int BARRIER_OPEN_ANGLE = 90;            // 차단기 열림 각도
const unsigned long GATE_OPEN_TIME_MS = 3000; // 차단기 자동 닫힘 대기 시간
const unsigned long LIGHT_LOG_INTERVAL_MS = 1000; // 조도센서 로그 출력 주기

SoftwareSerial wemosSerial(WEMOS_RX_PIN, WEMOS_TX_PIN); // Wemos 게이트웨이 통신 포트
LiquidCrystal_I2C lcd(0x27, 16, 2);                     // 주차장 상태 표시 LCD
Servo entranceBarrierServo;                             // 입구 차단기 제어 서보 객체
Servo exitBarrierServo;                                 // 출구 차단기 제어 서보 객체

int cachedEmptySlots = 2;                  // Wemos에서 마지막으로 받은 빈자리 수
bool previousEntranceDetected = false;     // 입구 감지 중복 처리 방지 상태
bool previousExitDetected = false;         // 출차 감지 중복 처리 방지 상태
unsigned long entranceGateOpenedAt = 0;    // 입구 차단기를 연 시각
unsigned long exitGateOpenedAt = 0;        // 출구 차단기를 연 시각
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
 * 지정한 차단기를 열린 각도로 이동한다.
 * @param {Servo&} barrierServo - 제어할 차단기 서보 객체
 * @returns {void} 반환값 없음
 */
void openGate(Servo& barrierServo) {
  barrierServo.write(BARRIER_OPEN_ANGLE);
}

/**
 * 지정한 차단기를 닫힌 각도로 유지한다.
 * @param {Servo&} barrierServo - 제어할 차단기 서보 객체
 * @returns {void} 반환값 없음
 */
void closeGate(Servo& barrierServo) {
  barrierServo.write(BARRIER_CLOSED_ANGLE);
}

/**
 * 만차 경고 장치를 켜거나 끈다.
 * @param {bool} enabled - 경고 활성화 여부
 * @returns {void} 반환값 없음
 */
void setFullAlert(bool enabled) {
  digitalWrite(FULL_ALERT_LED_PIN, enabled ? HIGH : LOW);
  digitalWrite(FULL_ALERT_BUZZER_PIN, enabled ? HIGH : LOW);
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
 * Wemos 게이트웨이에서 받은 한 줄 명령을 처리한다.
 * @param {String} line - Wemos가 전송한 명령 문자열
 * @returns {void} 반환값 없음
 */
void handleGatewayLine(String line) {
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
 * Wemos 게이트웨이에서 들어온 모든 명령을 읽는다.
 * @returns {void} 반환값 없음
 */
void readGatewayMessages() {
  while (wemosSerial.available()) {
    String line = wemosSerial.readStringUntil('\n');
    handleGatewayLine(line);
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
      setFullAlert(false);
      showMessage("Entrance Open", "Empty: " + String(cachedEmptySlots));
      openGate(entranceBarrierServo);
      entranceGateOpenedAt = millis();
    } else {
      setFullAlert(true);
      showMessage("Parking Full", "Gate Closed");
      closeGate(entranceBarrierServo);
      entranceGateOpenedAt = 0;
    }
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 출구 차량을 감지하면 Wemos 게이트웨이에 출차 확정을 알린다.
 * @param {int} exitLightValue - 출구 조도센서 값
 * @returns {void} 반환값 없음
 */
void handleExitVehicle(int exitLightValue) {
  bool exitDetected = isVehicleDetectedByLight(exitLightValue);

  if (exitDetected && !previousExitDetected) {
    wemosSerial.println("BARRIER_EXIT");
    showMessage("Exit Open", "Calculating...");
    openGate(exitBarrierServo);
    exitGateOpenedAt = millis();
  }

  previousExitDetected = exitDetected;
}

/**
 * 열린 차단기를 일정 시간 뒤 자동으로 닫는다.
 * @param {Servo&} barrierServo - 제어할 차단기 서보 객체
 * @param {unsigned long&} openedAt - 차단기를 연 시각
 * @returns {void} 반환값 없음
 */
void closeGateAfterTimeout(Servo& barrierServo, unsigned long& openedAt) {
  if (openedAt > 0 && millis() - openedAt >= GATE_OPEN_TIME_MS) {
    closeGate(barrierServo);
    openedAt = 0;
  }
}

/**
 * 입구/차단기 제어 보드를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  wemosSerial.begin(9600);

  pinMode(FULL_ALERT_LED_PIN, OUTPUT);
  pinMode(FULL_ALERT_BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  entranceBarrierServo.attach(ENTRANCE_SERVO_PIN);
  exitBarrierServo.attach(EXIT_SERVO_PIN);
  closeGate(entranceBarrierServo);
  closeGate(exitBarrierServo);
  showMessage("Smart Parking", "Gates Ready");
}

/**
 * 입구 감지, 차단기 통과 감지, 게이트웨이 메시지를 반복 처리한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  int entranceLightValue = analogRead(ENTRANCE_LIGHT_PIN);
  int exitLightValue = analogRead(EXIT_LIGHT_PIN);

  readGatewayMessages();
  logLightValues(entranceLightValue, exitLightValue);
  handleEntranceVehicle(entranceLightValue);
  handleExitVehicle(exitLightValue);
  closeGateAfterTimeout(entranceBarrierServo, entranceGateOpenedAt);
  closeGateAfterTimeout(exitBarrierServo, exitGateOpenedAt);
  delay(100);
}
