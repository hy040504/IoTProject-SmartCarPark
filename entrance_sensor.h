#ifndef ENTRANCE_SENSOR_H
#define ENTRANCE_SENSOR_H

#include <Arduino.h>

const int ENTRANCE_LIGHT_PIN = A0;            // 입구 조도센서 입력 핀
const int ENTRANCE_BLOCKED_THRESHOLD = 450;   // 차량 그림자로 판단할 조도 기준

/**
 * 입구 조도센서를 초기화한다.
 * @returns {void} 반환값 없음
 */
inline void setupEntranceSensor() {
  pinMode(ENTRANCE_LIGHT_PIN, INPUT);
}

/**
 * 입구 조도센서 값을 읽는다.
 * @returns {int} 조도센서 아날로그 값
 */
inline int readEntranceLightValue() {
  return analogRead(ENTRANCE_LIGHT_PIN);
}

/**
 * 조도 변화로 입구 차량 대기 여부를 판단한다.
 * @returns {bool} 입구에 차량이 감지되면 true, 아니면 false
 */
inline bool isEntranceVehicleDetected() {
  return readEntranceLightValue() <= ENTRANCE_BLOCKED_THRESHOLD;
}

/**
 * 차단기 통과 차량 감지 여부를 판단한다.
 * @returns {bool} 차단기에서 차량이 감지되면 true, 아니면 false
 */
inline bool isBarrierVehicleDetected() {
  return isEntranceVehicleDetected();
}

#endif
