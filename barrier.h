#ifndef BARRIER_H
#define BARRIER_H

#include <Arduino.h>
#include <Servo.h>

const int BARRIER_SERVO_PIN = D5;       // 차단기 서보모터 제어 핀
const int BARRIER_CLOSED_ANGLE = 0;     // 차단기 닫힘 각도
const int BARRIER_OPEN_ANGLE = 90;      // 차단기 열림 각도

static Servo barrierServo;

/**
 * 차단기 서보모터를 초기화하고 닫힘 상태로 둔다.
 * @returns {void} 반환값 없음
 */
inline void setupBarrier() {
  barrierServo.attach(BARRIER_SERVO_PIN);
  barrierServo.write(BARRIER_CLOSED_ANGLE);
}

/**
 * 빈자리가 있을 때 차단기를 열린 각도로 이동시킨다.
 * @returns {void} 반환값 없음
 */
inline void openBarrier() {
  barrierServo.write(BARRIER_OPEN_ANGLE);
}

/**
 * 만차이거나 대기 상태일 때 차단기를 닫힌 각도로 유지한다.
 * @returns {void} 반환값 없음
 */
inline void closeBarrier() {
  barrierServo.write(BARRIER_CLOSED_ANGLE);
}

#endif
