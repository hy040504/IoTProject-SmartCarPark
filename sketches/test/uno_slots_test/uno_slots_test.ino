#include <Arduino.h>

const int SLOT_COUNT = 2;                    // 테스트할 주차칸 개수
const unsigned long ECHO_TIMEOUT_US = 30000; // 초음파 응답 대기 최대 시간
const unsigned long SERIAL_LOG_MS = 500;     // 시리얼 로그 출력 주기
const unsigned long SENSOR_READ_DELAY_MS = 80; // 센서 간 간섭 완화 대기 시간
const unsigned long LED_TEST_STEP_MS = 350;  // LED 단독 점등 확인 시간

const int SLOT_OCCUPIED_DISTANCE_CM[SLOT_COUNT] = {8, 5}; // 주차칸별 차량 감지 거리
const int SLOT_TRIG_PINS[SLOT_COUNT] = {4, 13};      // 주차칸별 초음파 송신 핀
const int SLOT_ECHO_PINS[SLOT_COUNT] = {5, 12};      // 주차칸별 초음파 수신 핀
const int SLOT_RED_LED_PINS[SLOT_COUNT] = {8, 10};   // 주차칸별 점유 표시 LED 핀
const int SLOT_GREEN_LED_PINS[SLOT_COUNT] = {9, 11}; // 주차칸별 빈자리 표시 LED 핀

unsigned long lastSerialLoggedAt = 0; // 마지막 시리얼 로그 출력 시각

/**
 * 지정한 LED 핀을 켜고 끈다.
 * @param {int} pin - 제어할 LED 핀
 * @param {bool} enabled - LED 점등 여부
 * @returns {void} 반환값 없음
 */
void setLed(int pin, bool enabled) {
  digitalWrite(pin, enabled ? HIGH : LOW);
}

/**
 * 모든 주차칸 LED를 끈다.
 * @returns {void} 반환값 없음
 */
void turnOffAllLeds() {
  for (int index = 0; index < SLOT_COUNT; index++) {
    setLed(SLOT_RED_LED_PINS[index], false);
    setLed(SLOT_GREEN_LED_PINS[index], false);
  }
}

/**
 * 업로드 직후 LED 4개가 각각 켜지는지 순서대로 확인한다.
 * @returns {void} 반환값 없음
 */
void runStartupLedTest() {
  Serial.println("LED test: slot 1 red");
  setLed(SLOT_RED_LED_PINS[0], true);
  delay(LED_TEST_STEP_MS);
  turnOffAllLeds();

  Serial.println("LED test: slot 1 green");
  setLed(SLOT_GREEN_LED_PINS[0], true);
  delay(LED_TEST_STEP_MS);
  turnOffAllLeds();

  Serial.println("LED test: slot 2 red");
  setLed(SLOT_RED_LED_PINS[1], true);
  delay(LED_TEST_STEP_MS);
  turnOffAllLeds();

  Serial.println("LED test: slot 2 green");
  setLed(SLOT_GREEN_LED_PINS[1], true);
  delay(LED_TEST_STEP_MS);
  turnOffAllLeds();
}

/**
 * 지정한 주차칸의 초음파 거리를 센티미터 단위로 측정한다.
 * @param {int} slotIndex - 측정할 주차칸 배열 인덱스
 * @returns {float} 측정된 거리, 감지 실패 시 -1
 */
float readDistanceCm(int slotIndex) {
  digitalWrite(SLOT_TRIG_PINS[slotIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(SLOT_TRIG_PINS[slotIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(SLOT_TRIG_PINS[slotIndex], LOW);

  long duration = pulseIn(SLOT_ECHO_PINS[slotIndex], HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

/**
 * 측정 거리로 주차칸 점유 여부를 판단한다.
 * @param {int} slotIndex - 확인할 주차칸 배열 인덱스
 * @param {float} distanceCm - 초음파센서 측정 거리
 * @returns {bool} 차량이 있으면 true, 아니면 false
 */
bool isSlotOccupied(int slotIndex, float distanceCm) {
  return distanceCm > 0 && distanceCm <= SLOT_OCCUPIED_DISTANCE_CM[slotIndex];
}

/**
 * 주차칸 상태에 맞춰 빨간색/초록색 LED를 표시한다.
 * @param {int} slotIndex - 표시할 주차칸 배열 인덱스
 * @param {float} distanceCm - 초음파센서 측정 거리
 * @returns {void} 반환값 없음
 */
void updateSlotLed(int slotIndex, float distanceCm) {
  bool occupied = isSlotOccupied(slotIndex, distanceCm);
  bool sensorFailed = distanceCm < 0;

  setLed(SLOT_RED_LED_PINS[slotIndex], occupied);
  setLed(SLOT_GREEN_LED_PINS[slotIndex], !occupied && !sensorFailed);

  if (sensorFailed) {
    setLed(SLOT_RED_LED_PINS[slotIndex], true);
    setLed(SLOT_GREEN_LED_PINS[slotIndex], true);
  }
}

/**
 * 초음파센서 측정값과 판정 결과를 시리얼 모니터에 출력한다.
 * @param {float[]} distances - 주차칸별 측정 거리 배열
 * @returns {void} 반환값 없음
 */
void printSlotStatus(float distances[]) {
  if (millis() - lastSerialLoggedAt < SERIAL_LOG_MS) {
    return;
  }

  lastSerialLoggedAt = millis();

  for (int index = 0; index < SLOT_COUNT; index++) {
    Serial.print("Slot ");
    Serial.print(index + 1);
    Serial.print(": ");

    if (distances[index] < 0) {
      Serial.print("no echo");
    } else {
      Serial.print(distances[index]);
      Serial.print(" cm, ");
      Serial.print(isSlotOccupied(index, distances[index]) ? "occupied" : "empty");
    }

    if (index < SLOT_COUNT - 1) {
      Serial.print(" | ");
    }
  }

  Serial.println();
}

/**
 * 주차칸 테스트 시작 정보를 시리얼 모니터에 출력한다.
 * @returns {void} 반환값 없음
 */
void printBootMessage() {
  Serial.println();
  Serial.println("=== Uno Slots Hardware Test Start ===");
  Serial.println("Baud: 9600");
  Serial.println("Slot 1: TRIG D4, ECHO D5, RED D8, GREEN D9");
  Serial.println("Slot 2: TRIG D13, ECHO D12, RED D10, GREEN D11");
  Serial.println("Slot 1 threshold: 8 cm");
  Serial.println("Slot 2 threshold: 5 cm");
}

/**
 * 초음파센서 2개와 LED 4개 테스트를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  delay(1000);
  printBootMessage();

  for (int index = 0; index < SLOT_COUNT; index++) {
    pinMode(SLOT_TRIG_PINS[index], OUTPUT);
    pinMode(SLOT_ECHO_PINS[index], INPUT);
    pinMode(SLOT_RED_LED_PINS[index], OUTPUT);
    pinMode(SLOT_GREEN_LED_PINS[index], OUTPUT);
  }

  turnOffAllLeds();
  runStartupLedTest();
}

/**
 * 초음파 거리와 LED 상태 표시를 반복 테스트한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  float distances[SLOT_COUNT];

  for (int index = 0; index < SLOT_COUNT; index++) {
    distances[index] = readDistanceCm(index);
    updateSlotLed(index, distances[index]);
    delay(SENSOR_READ_DELAY_MS);
  }

  printSlotStatus(distances);
}
