#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

const int WEMOS_RX_PIN = 2;
const int WEMOS_TX_PIN = 3;
const int ENTRANCE_LIGHT_PIN = A0;
const int BARRIER_EXIT_LIGHT_PIN = A1;
const int SERVO_PIN = 9;
const int FULL_ALERT_LED_PIN = 12;
const int FULL_ALERT_BUZZER_PIN = 11;

const int LIGHT_BLOCKED_THRESHOLD = 450;
const int BARRIER_CLOSED_ANGLE = 0;
const int BARRIER_OPEN_ANGLE = 90;
const unsigned long GATE_OPEN_TIME_MS = 3000;

SoftwareSerial wemosSerial(WEMOS_RX_PIN, WEMOS_TX_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo barrierServo;

int cachedEmptySlots = 2;
bool previousEntranceDetected = false;
bool previousExitDetected = false;
unsigned long gateOpenedAt = 0;

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
 * 차단기를 열린 각도로 이동하고 자동 닫힘 시간을 기록한다.
 * @returns {void} 반환값 없음
 */
void openGate() {
  barrierServo.write(BARRIER_OPEN_ANGLE);
  gateOpenedAt = millis();
}

/**
 * 차단기를 닫힌 각도로 유지한다.
 * @returns {void} 반환값 없음
 */
void closeGate() {
  barrierServo.write(BARRIER_CLOSED_ANGLE);
  gateOpenedAt = 0;
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
 * @param {int} pin - 확인할 조도센서 핀
 * @returns {bool} 차량이 감지되면 true, 아니면 false
 */
bool isVehicleDetectedByLight(int pin) {
  return analogRead(pin) <= LIGHT_BLOCKED_THRESHOLD;
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
void handleEntranceVehicle() {
  bool entranceDetected = isVehicleDetectedByLight(ENTRANCE_LIGHT_PIN);

  if (entranceDetected && !previousEntranceDetected) {
    if (cachedEmptySlots > 0) {
      setFullAlert(false);
      showMessage("Gate Open", "Empty: " + String(cachedEmptySlots));
      openGate();
    } else {
      setFullAlert(true);
      showMessage("Parking Full", "Gate Closed");
      closeGate();
    }
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 차단기 통과 차량을 감지하면 Wemos 게이트웨이에 출차 확정을 알린다.
 * @returns {void} 반환값 없음
 */
void handleBarrierExitVehicle() {
  bool exitDetected = isVehicleDetectedByLight(BARRIER_EXIT_LIGHT_PIN);

  if (exitDetected && !previousExitDetected) {
    wemosSerial.println("BARRIER_EXIT");
    openGate();
  }

  previousExitDetected = exitDetected;
}

/**
 * 열린 차단기를 일정 시간 뒤 자동으로 닫는다.
 * @returns {void} 반환값 없음
 */
void closeGateAfterTimeout() {
  if (gateOpenedAt > 0 && millis() - gateOpenedAt >= GATE_OPEN_TIME_MS) {
    closeGate();
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
  barrierServo.attach(SERVO_PIN);
  closeGate();
  showMessage("Smart Parking", "Gate Ready");
}

/**
 * 입구 감지, 차단기 통과 감지, 게이트웨이 메시지를 반복 처리한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  readGatewayMessages();
  handleEntranceVehicle();
  handleBarrierExitVehicle();
  closeGateAfterTimeout();
  delay(100);
}
