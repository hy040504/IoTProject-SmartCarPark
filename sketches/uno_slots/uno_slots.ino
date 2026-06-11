#include <SoftwareSerial.h>

const int WEMOS_RX_PIN = 2;                  // Wemos 송신선을 받는 핀
const int WEMOS_TX_PIN = 3;                  // Wemos 수신선으로 보내는 핀
const int SLOT_COUNT = 2;                    // 관리할 주차칸 개수
const int SLOT_OCCUPIED_DISTANCE_CM = 5;     // 차량으로 판단할 최대 거리
const unsigned long ECHO_TIMEOUT_US = 30000; // 초음파 응답 대기 최대 시간

const int SLOT_TRIG_PINS[SLOT_COUNT] = {4, 6};       // 주차칸별 초음파 송신 핀
const int SLOT_ECHO_PINS[SLOT_COUNT] = {5, 7};       // 주차칸별 초음파 수신 핀
const int SLOT_RED_LED_PINS[SLOT_COUNT] = {8, 10};   // 주차칸별 점유 표시 LED 핀
const int SLOT_GREEN_LED_PINS[SLOT_COUNT] = {9, 11}; // 주차칸별 빈자리 표시 LED 핀

SoftwareSerial wemosSerial(WEMOS_RX_PIN, WEMOS_TX_PIN); // Wemos 게이트웨이 통신 포트
bool previousOccupied[SLOT_COUNT] = {false, false};     // 주차칸 상태 변화 감지용 이전 상태

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
 * 지정한 주차칸이 점유 상태인지 판단한다.
 * @param {int} slotIndex - 확인할 주차칸 배열 인덱스
 * @returns {bool} 차량이 있으면 true, 아니면 false
 */
bool isSlotOccupied(int slotIndex) {
  float distanceCm = readDistanceCm(slotIndex);
  return distanceCm > 0 && distanceCm <= SLOT_OCCUPIED_DISTANCE_CM;
}

/**
 * 주차칸 LED를 점유 상태에 맞게 갱신한다.
 * @param {int} slotIndex - 갱신할 주차칸 배열 인덱스
 * @param {bool} occupied - 주차칸 점유 여부
 * @returns {void} 반환값 없음
 */
void updateSlotLed(int slotIndex, bool occupied) {
  digitalWrite(SLOT_RED_LED_PINS[slotIndex], occupied ? HIGH : LOW);
  digitalWrite(SLOT_GREEN_LED_PINS[slotIndex], occupied ? LOW : HIGH);
}

/**
 * Wemos 게이트웨이에 주차칸 상태 변화 이벤트를 전송한다.
 * @param {String} eventName - ENTRY 또는 VACATED 이벤트 이름
 * @param {int} slotId - 주차칸 번호
 * @returns {void} 반환값 없음
 */
void sendSlotEvent(String eventName, int slotId) {
  wemosSerial.println(eventName + "," + String(slotId));
}

/**
 * 각 주차칸의 점유 변화와 LED 표시를 처리한다.
 * @returns {void} 반환값 없음
 */
void updateParkingSlots() {
  for (int index = 0; index < SLOT_COUNT; index++) {
    bool occupied = isSlotOccupied(index);
    int slotId = index + 1;

    updateSlotLed(index, occupied);

    if (occupied && !previousOccupied[index]) {
      sendSlotEvent("ENTRY", slotId);
    }

    if (!occupied && previousOccupied[index]) {
      sendSlotEvent("VACATED", slotId);
    }

    previousOccupied[index] = occupied;
  }
}

/**
 * 주차칸 감지 보드를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  wemosSerial.begin(9600);

  for (int index = 0; index < SLOT_COUNT; index++) {
    pinMode(SLOT_TRIG_PINS[index], OUTPUT);
    pinMode(SLOT_ECHO_PINS[index], INPUT);
    pinMode(SLOT_RED_LED_PINS[index], OUTPUT);
    pinMode(SLOT_GREEN_LED_PINS[index], OUTPUT);
  }
}

/**
 * 주차칸 상태를 반복 감지하고 Wemos 게이트웨이에 변화 이벤트를 보낸다.
 * @returns {void} 반환값 없음
 */
void loop() {
  updateParkingSlots();
  delay(300);
}
