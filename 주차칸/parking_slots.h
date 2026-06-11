#ifndef PARKING_SLOTS_H
#define PARKING_SLOTS_H

#include <Arduino.h>
#include "../요금/fee_request.h"
#include "parking_display.h"

const int SLOT_COUNT = 2;                       // 관리할 주차칸 개수
const int SLOT_OCCUPIED_DISTANCE_CM = 5;        // 차량으로 판단할 최대 거리
const unsigned long SLOT_ECHO_TIMEOUT_US = 30000; // 초음파 응답 대기 최대 시간

const int SLOT_TRIG_PINS[SLOT_COUNT] = {D6, D7};   // 주차칸별 초음파 송신 핀
const int SLOT_ECHO_PINS[SLOT_COUNT] = {D3, D4};   // 주차칸별 초음파 수신 핀
const int SLOT_RED_LED_PINS[SLOT_COUNT] = {D9, D11};   // 주차칸별 점유 표시 LED 핀
const int SLOT_GREEN_LED_PINS[SLOT_COUNT] = {D10, D12}; // 주차칸별 빈자리 표시 LED 핀

static bool previousSlotOccupied[SLOT_COUNT] = {false, false};
static int pendingExitSlotId = 0;
static bool previousBarrierExitDetected = false;

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
  return distanceCm > 0 && distanceCm <= SLOT_OCCUPIED_DISTANCE_CM;
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
 * 차량이 빠진 주차칸을 출차 대기 상태로 저장한다.
 * @param {int} slotId - 차량이 빠진 주차칸 번호
 * @returns {void} 반환값 없음
 */
inline void markPendingExitSlot(int slotId) {
  pendingExitSlotId = slotId;
}

/**
 * 출차 확정을 기다리는 주차칸이 있는지 확인한다.
 * @returns {bool} 출차 대기 중인 주차칸이 있으면 true, 아니면 false
 */
inline bool hasPendingExitSlot() {
  return pendingExitSlotId > 0;
}

/**
 * 주차칸 점유 변화에 따라 입차와 출차 대기 이벤트를 처리한다.
 * @returns {void} 반환값 없음
 */
inline void updateParkingSlots() {
  for (int index = 0; index < SLOT_COUNT; index++) {
    bool occupied = isSlotOccupied(index);
    int slotId = index + 1;

    updateSlotLed(index, occupied);

    if (occupied && !previousSlotOccupied[index]) {
      sendVehicleEntryRequest(slotId);
    }

    if (!occupied && previousSlotOccupied[index]) {
      markPendingExitSlot(slotId);
    }

    previousSlotOccupied[index] = occupied;
  }
}

/**
 * 차단기 감지까지 확인된 출차 이벤트를 요금 서버에 전송한다.
 * @param {bool} barrierExitDetected - 차단기에서 차량 통과가 감지됐는지 여부
 * @returns {bool} 출차 요금 계산이 완료되면 true, 아니면 false
 */
inline bool confirmPendingExitAtBarrier(bool barrierExitDetected) {
  bool exitConfirmed = hasPendingExitSlot() && barrierExitDetected && !previousBarrierExitDetected;

  previousBarrierExitDetected = barrierExitDetected;

  if (!exitConfirmed) {
    return false;
  }

  int slotId = pendingExitSlotId;
  String response = sendVehicleExitRequest(slotId);
  long fee = extractFeeFromResponse(response);

  pendingExitSlotId = 0;

  if (fee < 0) {
    return false;
  }

  showParkingFeeMessage(slotId, fee);
  return true;
}

#endif
