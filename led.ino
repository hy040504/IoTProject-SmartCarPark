#include "barrier.h"
#include "entrance_sensor.h"
#include "parking_alert.h"
#include "parking_display.h"
#include "parking_slots.h"

/**
 * 주차장 감지 시스템을 시작한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  setupParkingFeeClient();
  setupEntranceSensor();
  setupParkingDisplay();
  setupParkingAlert();
  setupParkingSlots();
  setupBarrier();
}

/**
 * 주차 공간 상태를 반복 측정하고 LED 표시를 갱신한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  bool entranceVehicleDetected = isEntranceVehicleDetected();
  bool barrierVehicleDetected = isBarrierVehicleDetected();
  int emptySlots = countEmptySlots();

  updateParkingSlots();
  bool exitFeeDisplayed = confirmPendingExitAtBarrier(barrierVehicleDetected);

  if (exitFeeDisplayed) {
    turnOffParkingFullAlert();
    openBarrier();
  } else if (entranceVehicleDetected && emptySlots > 0) {
    showGateOpenMessage(emptySlots);
    turnOffParkingFullAlert();
    openBarrier();
  } else if (entranceVehicleDetected && emptySlots == 0) {
    showParkingFullMessage();
    turnOnParkingFullAlert();
    closeBarrier();
  } else {
    turnOffParkingFullAlert();
    closeBarrier();
  }

  Serial.print("Entrance: ");
  Serial.print(entranceVehicleDetected ? "YES" : "NO");
  Serial.print(", Empty Slots: ");
  Serial.println(emptySlots);

  delay(300);
}
