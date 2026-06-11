#ifndef PARKING_ALERT_H
#define PARKING_ALERT_H

#include <Arduino.h>

const int FULL_ALERT_LED_PIN = D0;      // 만차 경고 LED 핀
const int FULL_ALERT_BUZZER_PIN = D8;   // 만차 경고 부저 핀

/**
 * 만차 경고 장치를 초기화한다.
 * @returns {void} 반환값 없음
 */
inline void setupParkingAlert() {
  pinMode(FULL_ALERT_LED_PIN, OUTPUT);
  pinMode(FULL_ALERT_BUZZER_PIN, OUTPUT);
}

/**
 * 만차 경고를 켠다.
 * @returns {void} 반환값 없음
 */
inline void turnOnParkingFullAlert() {
  digitalWrite(FULL_ALERT_LED_PIN, HIGH);
  digitalWrite(FULL_ALERT_BUZZER_PIN, HIGH);
}

/**
 * 만차 경고를 끈다.
 * @returns {void} 반환값 없음
 */
inline void turnOffParkingFullAlert() {
  digitalWrite(FULL_ALERT_LED_PIN, LOW);
  digitalWrite(FULL_ALERT_BUZZER_PIN, LOW);
}

#endif
