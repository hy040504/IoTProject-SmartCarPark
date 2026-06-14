#ifndef PARKING_LCD_DISPLAY_H
#define PARKING_LCD_DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const int PARKING_TOTAL_SLOTS = 2;

/**
 * LCD에 표시할 문자열을 16칸에 맞춘다.
 * @param {String} text - 출력할 문자열
 * @returns {String} 16칸으로 맞춘 문자열
 */
inline String fitLcdLine(String text) {
  if (text.length() > 16) {
    return text.substring(0, 16);
  }

  while (text.length() < 16) {
    text += " ";
  }

  return text;
}

/**
 * 지정한 LCD의 두 줄을 갱신한다.
 * @param {LiquidCrystal_I2C&} lcd - 대상 LCD
 * @param {String} firstLine - 첫 번째 줄
 * @param {String} secondLine - 두 번째 줄
 * @returns {void} 반환값 없음
 */
inline void showLcdMessage(LiquidCrystal_I2C& lcd, const String& firstLine, const String& secondLine) {
  lcd.setCursor(0, 0);
  lcd.print(fitLcdLine(firstLine));
  lcd.setCursor(0, 1);
  lcd.print(fitLcdLine(secondLine));
}

/**
 * 출구 LCD에 주차 현황을 표시한다.
 * @param {LiquidCrystal_I2C&} lcd - 대상 LCD
 * @param {int} occupiedSlots - 현재 점유 칸 수
 * @param {int} totalSlots - 전체 칸 수
 * @returns {void} 반환값 없음
 */
inline void showParkingStatus(LiquidCrystal_I2C& lcd, int occupiedSlots, int totalSlots) {
  showLcdMessage(lcd, "Parking Status", "Cars: " + String(occupiedSlots) + "/" + String(totalSlots));
}

/**
 * 출차 요금을 출구 LCD에 표시한다.
 * @param {LiquidCrystal_I2C&} lcd - 대상 LCD
 * @param {int} slotId - 주차칸 번호
 * @param {long} fee - 계산된 요금
 * @returns {void} 반환값 없음
 */
inline void showExitFee(LiquidCrystal_I2C& lcd, int slotId, long fee) {
  showLcdMessage(lcd, "Slot " + String(slotId) + " Exit", "Fee: " + String(fee));
}

#endif
