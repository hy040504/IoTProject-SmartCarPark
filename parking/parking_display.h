#ifndef PARKING_DISPLAY_H
#define PARKING_DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const int LCD_ADDRESS = 0x27;           // I2C LCD 기본 주소
const int LCD_COLUMNS = 16;             // LCD 가로 칸 수
const int LCD_ROWS = 2;                 // LCD 세로 줄 수

static LiquidCrystal_I2C parkingLcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
static String currentFirstLine = "";
static String currentSecondLine = "";

/**
 * 주차장 상태 표시용 LCD를 초기화한다.
 * @returns {void} 반환값 없음
 */
inline void setupParkingDisplay() {
  parkingLcd.init();
  parkingLcd.backlight();
  parkingLcd.clear();
}

/**
 * LCD 두 줄에 메시지를 출력한다.
 * @param {String} firstLine - 첫 번째 줄 메시지
 * @param {String} secondLine - 두 번째 줄 메시지
 * @returns {void} 반환값 없음
 */
inline void showParkingMessage(String firstLine, String secondLine) {
  firstLine = firstLine.substring(0, LCD_COLUMNS);
  secondLine = secondLine.substring(0, LCD_COLUMNS);

  if (firstLine == currentFirstLine && secondLine == currentSecondLine) {
    return;
  }

  currentFirstLine = firstLine;
  currentSecondLine = secondLine;

  parkingLcd.clear();
  parkingLcd.setCursor(0, 0);
  parkingLcd.print(firstLine);
  parkingLcd.setCursor(0, 1);
  parkingLcd.print(secondLine);
}

/**
 * 차단기 열림 상태를 LCD에 표시한다.
 * @param {int} emptySlots - 현재 빈자리 개수
 * @returns {void} 반환값 없음
 */
inline void showGateOpenMessage(int emptySlots) {
  showParkingMessage("Gate Open", "Empty: " + String(emptySlots));
}

/**
 * 만차 상태를 LCD에 표시한다.
 * @returns {void} 반환값 없음
 */
inline void showParkingFullMessage() {
  showParkingMessage("Parking Full", "Gate Closed");
}

/**
 * 출차 시 계산된 요금을 LCD에 표시한다.
 * @param {int} slotId - 출차한 주차칸 번호
 * @param {long} fee - 계산된 주차 요금
 * @returns {void} 반환값 없음
 */
inline void showParkingFeeMessage(int slotId, long fee) {
  showParkingMessage("Slot " + String(slotId) + " Exit", "Fee: " + String(fee));
}

#endif
