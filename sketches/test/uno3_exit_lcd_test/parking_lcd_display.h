#ifndef PARKING_LCD_DISPLAY_H
#define PARKING_LCD_DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const int PARKING_TOTAL_SLOTS = 2;
const int LCD_COLUMNS = 20;
const int LCD_ROWS = 4;

/**
 * LCD에 표시할 문자열을 LCD 폭에 맞춘다.
 * @param {String} text - 출력할 문자열
 * @returns {String} LCD 폭으로 맞춘 문자열
 */
inline String fitLcdLine(String text) {
  if (text.length() > LCD_COLUMNS) {
    return text.substring(0, LCD_COLUMNS);
  }

  while (text.length() < LCD_COLUMNS) {
    text += " ";
  }

  return text;
}

/**
 * 지정한 LCD의 네 줄을 갱신한다.
 * @param {LiquidCrystal_I2C&} lcd - 대상 LCD
 * @param {String} firstLine - 첫 번째 줄
 * @param {String} secondLine - 두 번째 줄
 * @param {String} thirdLine - 세 번째 줄
 * @param {String} fourthLine - 네 번째 줄
 * @returns {void} 반환값 없음
 */
inline void showLcdMessage(
  LiquidCrystal_I2C& lcd,
  const String& firstLine,
  const String& secondLine,
  const String& thirdLine,
  const String& fourthLine
) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(fitLcdLine(firstLine));
  lcd.setCursor(0, 1);
  lcd.print(fitLcdLine(secondLine));
  lcd.setCursor(0, 2);
  lcd.print(fitLcdLine(thirdLine));
  lcd.setCursor(0, 3);
  lcd.print(fitLcdLine(fourthLine));
}

inline String formatParkedDuration(unsigned long parkedSeconds) {
  unsigned long hours = parkedSeconds / 3600;
  unsigned long minutes = (parkedSeconds % 3600) / 60;
  unsigned long seconds = parkedSeconds % 60;

  return "Parked " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
}

/**
 * 출구 LCD에 주차 현황을 표시한다.
 * @param {LiquidCrystal_I2C&} lcd - 대상 LCD
 * @param {int} occupiedSlots - 현재 점유 칸 수
 * @param {int} totalSlots - 전체 칸 수
 * @returns {void} 반환값 없음
 */
inline void showParkingStatus(LiquidCrystal_I2C& lcd, int occupiedSlots, int totalSlots) {
  showLcdMessage(
    lcd,
    "Date / Time",
    "Waiting for data",
    "Smart Parking Team",
    "Cars: " + String(occupiedSlots) + "/" + String(totalSlots) + " | YSH JKH"
  );
}

/**
 * 출차 요금을 출구 LCD에 표시한다.
 * @param {LiquidCrystal_I2C&} lcd - 대상 LCD
 * @param {int} slotId - 주차칸 번호
 * @param {long} fee - 계산된 요금
 * @returns {void} 반환값 없음
 */
inline void showExitFee(
  LiquidCrystal_I2C& lcd,
  int slotId,
  long fee,
  const String& exitTime,
  unsigned long parkedSeconds
) {
  showLcdMessage(
    lcd,
    "Exit " + exitTime,
    "Slot " + String(slotId) + " Fee " + String(fee) + " KRW",
    formatParkedDuration(parkedSeconds),
    "Thank you. Bye!"
  );
}

#endif
