#ifndef PARKING_SLOTS_H
#define PARKING_SLOTS_H

#include <Arduino.h>

const int SLOT_COUNT = 2;                       // 관리할 주차칸 개수
const int SLOT_OCCUPIED_DISTANCE_CM[SLOT_COUNT] = {16, 10}; // 주차칸별 차량 감지 거리
const unsigned long SLOT_ECHO_TIMEOUT_US = 30000; // 초음파 응답 대기 최대 시간

const int SLOT_TRIG_PINS[SLOT_COUNT] = {4, 13};     // 주차칸별 초음파 송신 핀
const int SLOT_ECHO_PINS[SLOT_COUNT] = {5, 12};     // 주차칸별 초음파 수신 핀
const int SLOT_RED_LED_PINS[SLOT_COUNT] = {8, 10};  // 주차칸별 점유 표시 LED 핀
const int SLOT_GREEN_LED_PINS[SLOT_COUNT] = {9, 11}; // 주차칸별 빈자리 표시 LED 핀

static bool previousSlotOccupied[SLOT_COUNT] = {false, false};

/**
 * 주차칸 초음파 센서와 상태 LED를 초기화한다.
 * @returns {void} 반환값 없음
 */
inline void setupParkingSlots() {
  for (int index = 0; index < SLOT_COUNT; index++) {
    pinMode(SLOT_TRIG_PINS[index], OUTPUT);
    pinMode(SLOT_ECHO_PINS[index], INPUT);
    pinMode(SLOT_RED_LED_PINS[index], OUTPUT);
    pinMode(SLOT_GREEN_LED_PINS[index], OUTPUT);
  }
}

/**
 * 지정한 주차칸의 초음파 거리를 센티미터 단위로 측정한다.
 * @param {int} slotIndex - 측정할 주차칸 배열 인덱스
 * @returns {float} 측정된 거리, 감지 실패 시 -1
 */
inline float readSlotDistanceCm(int slotIndex) {
  digitalWrite(SLOT_TRIG_PINS[slotIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(SLOT_TRIG_PINS[slotIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(SLOT_TRIG_PINS[slotIndex], LOW);

  long duration = pulseIn(SLOT_ECHO_PINS[slotIndex], HIGH, SLOT_ECHO_TIMEOUT_US);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

/**
 * 지정한 주차칸이 차량으로 점유됐는지 판단한다.
 * @param {int} slotIndex - 확인할 주차칸 배열 인덱스
 * @returns {bool} 차량이 감지되면 true, 아니면 false
 */
inline bool isSlotOccupied(int slotIndex) {
  float distanceCm = readSlotDistanceCm(slotIndex);
  return distanceCm > 0 && distanceCm <= SLOT_OCCUPIED_DISTANCE_CM[slotIndex];
}

/**
 * 지정한 주차칸의 LED 상태를 갱신한다.
 * @param {int} slotIndex - 갱신할 주차칸 배열 인덱스
 * @param {bool} occupied - 주차칸 점유 여부
 * @returns {void} 반환값 없음
 */
inline void updateSlotLed(int slotIndex, bool occupied) {
  digitalWrite(SLOT_RED_LED_PINS[slotIndex], occupied ? HIGH : LOW);
  digitalWrite(SLOT_GREEN_LED_PINS[slotIndex], occupied ? LOW : HIGH);
}

/**
 * 현재 빈 주차칸 개수를 계산한다.
 * @returns {int} 빈 주차칸 개수
 */
inline int countEmptySlots() {
  int emptyCount = 0;

  for (int index = 0; index < SLOT_COUNT; index++) {
    if (!isSlotOccupied(index)) {
      emptyCount++;
    }
  }

  return emptyCount;
}

/**
 * Node.js 서버가 처리할 주차칸 상태 변경 이벤트를 Serial로 전송한다.
 * @param {const char*} eventName - ENTRY 또는 VACATED 이벤트 이름
 * @param {int} slotId - 주차칸 번호
 * @returns {void} 반환값 없음
 */
inline void sendSlotSerialEvent(const char* eventName, int slotId) {
  Serial.print(eventName);
  Serial.print(",");
  Serial.println(slotId);
}

/**
 * 주차칸 점유 변화에 따라 Node.js 서버로 보낼 Serial 이벤트를 처리한다.
 * @returns {void} 반환값 없음
 */
inline void updateParkingSlots() {
  for (int index = 0; index < SLOT_COUNT; index++) {
    bool occupied = isSlotOccupied(index);
    int slotId = index + 1;

    updateSlotLed(index, occupied);

    if (occupied && !previousSlotOccupied[index]) {
      sendSlotSerialEvent("ENTRY", slotId);
    }

    if (!occupied && previousSlotOccupied[index]) {
      sendSlotSerialEvent("VACATED", slotId);
    }

    previousSlotOccupied[index] = occupied;
  }
}

#endif
