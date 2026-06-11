#include <LiquidCrystal_I2C.h>
#include <Servo.h>

const int ENTRANCE_LIGHT_PIN = A0;      // 입구 조도센서 핀
const int EXIT_LIGHT_PIN = A1;          // 출구 조도센서 핀
const int ENTRANCE_SERVO_PIN = 9;       // 입구 차단기 서보 핀
const int EXIT_SERVO_PIN = 10;          // 출구 차단기 서보 핀

const int LIGHT_BLOCKED_THRESHOLD = 350;      // 차량 감지 조도 기준
const int BARRIER_CLOSED_ANGLE = 0;           // 차단기 닫힘 각도
const int BARRIER_OPEN_ANGLE = 90;            // 차단기 열림 각도
const unsigned long GATE_OPEN_TIME_MS = 2000; // 테스트용 차단기 열림 시간
const unsigned long LCD_UPDATE_MS = 500;      // LCD 갱신 주기

LiquidCrystal_I2C lcd(0x27, 16, 2); // 테스트 상태 표시 LCD
Servo entranceServo;                // 입구 차단기 서보 객체
Servo exitServo;                    // 출구 차단기 서보 객체

bool previousEntranceDetected = false;  // 입구 감지 중복 동작 방지 상태
bool previousExitDetected = false;      // 출구 감지 중복 동작 방지 상태
unsigned long entranceOpenedAt = 0;     // 입구 차단기를 연 시각
unsigned long exitOpenedAt = 0;         // 출구 차단기를 연 시각
unsigned long lastLcdUpdatedAt = 0;     // 마지막 LCD 갱신 시각

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
 * 조도센서 값으로 차량 감지 여부를 판단한다.
 * @param {int} lightValue - 조도센서 값
 * @returns {bool} 기준값 이하이면 true, 아니면 false
 */
bool isVehicleDetected(int lightValue) {
  return lightValue <= LIGHT_BLOCKED_THRESHOLD;
}

/**
 * 지정한 서보 차단기를 연다.
 * @param {Servo&} servo - 열 차단기 서보 객체
 * @returns {void} 반환값 없음
 */
void openBarrier(Servo& servo) {
  servo.write(BARRIER_OPEN_ANGLE);
}

/**
 * 지정한 서보 차단기를 닫는다.
 * @param {Servo&} servo - 닫을 차단기 서보 객체
 * @returns {void} 반환값 없음
 */
void closeBarrier(Servo& servo) {
  servo.write(BARRIER_CLOSED_ANGLE);
}

/**
 * 열린 차단기를 테스트 시간 이후 자동으로 닫는다.
 * @param {Servo&} servo - 제어할 차단기 서보 객체
 * @param {unsigned long&} openedAt - 차단기를 연 시각
 * @returns {void} 반환값 없음
 */
void closeBarrierAfterTimeout(Servo& servo, unsigned long& openedAt) {
  if (openedAt > 0 && millis() - openedAt >= GATE_OPEN_TIME_MS) {
    closeBarrier(servo);
    openedAt = 0;
  }
}

/**
 * 입구 조도센서와 입구 차단기 동작을 테스트한다.
 * @param {int} entranceLightValue - 입구 조도센서 값
 * @returns {void} 반환값 없음
 */
void testEntranceBarrier(int entranceLightValue) {
  bool entranceDetected = isVehicleDetected(entranceLightValue);

  if (entranceDetected && !previousEntranceDetected) {
    openBarrier(entranceServo);
    entranceOpenedAt = millis();
    showMessage("Entrance Test", "Servo Open");
  }

  previousEntranceDetected = entranceDetected;
}

/**
 * 출구 조도센서와 출구 차단기 동작을 테스트한다.
 * @param {int} exitLightValue - 출구 조도센서 값
 * @returns {void} 반환값 없음
 */
void testExitBarrier(int exitLightValue) {
  bool exitDetected = isVehicleDetected(exitLightValue);

  if (exitDetected && !previousExitDetected) {
    openBarrier(exitServo);
    exitOpenedAt = millis();
    showMessage("Exit Test", "Servo Open");
  }

  previousExitDetected = exitDetected;
}

/**
 * 조도센서 값을 시리얼 모니터와 LCD에 출력한다.
 * @param {int} entranceLightValue - 입구 조도센서 값
 * @param {int} exitLightValue - 출구 조도센서 값
 * @returns {void} 반환값 없음
 */
void printLightValues(int entranceLightValue, int exitLightValue) {
  Serial.print("Entrance: ");
  Serial.print(entranceLightValue);
  Serial.print(", Exit: ");
  Serial.println(exitLightValue);

  if (millis() - lastLcdUpdatedAt < LCD_UPDATE_MS) {
    return;
  }

  lastLcdUpdatedAt = millis();
  showMessage("IN:" + String(entranceLightValue), "OUT:" + String(exitLightValue));
}

/**
 * 부착된 입구/출구 조도센서, LCD, 서보모터를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  entranceServo.attach(ENTRANCE_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);
  closeBarrier(entranceServo);
  closeBarrier(exitServo);

  showMessage("Gate Test", "Ready");
}

/**
 * 조도센서 값과 차단기 서보 동작을 반복 테스트한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  int entranceLightValue = analogRead(ENTRANCE_LIGHT_PIN);
  int exitLightValue = analogRead(EXIT_LIGHT_PIN);

  printLightValues(entranceLightValue, exitLightValue);
  testEntranceBarrier(entranceLightValue);
  testExitBarrier(exitLightValue);
  closeBarrierAfterTimeout(entranceServo, entranceOpenedAt);
  closeBarrierAfterTimeout(exitServo, exitOpenedAt);

  delay(100);
}
