#ifndef PARKING_LCD_DISPLAY_H
#define PARKING_LCD_DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const int PARKING_TOTAL_SLOTS = 2; // 표시할 전체 주차칸 수

/**
 * LCD에 출력할 문자열을 16글자 폭에 맞춰 정리한다.
 * @param {String} text - LCD에 표시할 원본 문자열
 * @returns {String} 16글자 이내로 정리된 문자열
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
 * @param {LiquidCrystal_I2C&} lcd - 갱신할 LCD 객체
 * @param {String} firstLine - 첫 번째 줄 메시지
 * @param {String} secondLine - 두 번째 줄 메시지
 * @returns {void} 반환값 없음
 */
inline void showLcdMessage(LiquidCrystal_I2C& lcd, String firstLine, String secondLine) {
  lcd.setCursor(0, 0);
  lcd.print(fitLcdLine(firstLine));
  lcd.setCursor(0, 1);
  lcd.print(fitLcdLine(secondLine));
}

/**
 * 입구와 출구 LCD의 기본 주차 현황 화면을 표시한다.
 * @param {LiquidCrystal_I2C&} entranceLcd - 입구 안내 LCD 객체
 * @param {LiquidCrystal_I2C&} exitLcd - 출구 안내 LCD 객체
 * @param {int} occupiedSlots - 현재 주차 중인 차량 수
 * @param {int} totalSlots - 전체 주차칸 수
 * @returns {void} 반환값 없음
 */
inline void showParkingStatus(LiquidCrystal_I2C& entranceLcd, LiquidCrystal_I2C& exitLcd, int occupiedSlots, int totalSlots) {
  String summary = "Cars: " + String(occupiedSlots) + "/" + String(totalSlots);
  showLcdMessage(entranceLcd, "Parking Status", summary);
  showLcdMessage(exitLcd, "Parking Status", summary);
}

/**
 * 입구 LCD에 입차 가능 안내를 표시한다.
 * @param {LiquidCrystal_I2C&} entranceLcd - 입구 안내 LCD 객체
 * @param {int} emptySlots - 현재 빈 주차칸 수
 * @returns {void} 반환값 없음
 */
inline void showEntranceWelcome(LiquidCrystal_I2C& entranceLcd, int emptySlots) {
  showLcdMessage(entranceLcd, "Welcome", "Empty: " + String(emptySlots));
}

/**
 * 입구 LCD에 만차 안내를 표시한다.
 * @param {LiquidCrystal_I2C&} entranceLcd - 입구 안내 LCD 객체
 * @returns {void} 반환값 없음
 */
inline void showParkingFull(LiquidCrystal_I2C& entranceLcd) {
  showLcdMessage(entranceLcd, "Parking Full", "Please wait");
}

/**
 * 출구 LCD에 출차 요금을 표시한다.
 * @param {LiquidCrystal_I2C&} exitLcd - 출구 안내 LCD 객체
 * @param {int} slotId - 출차한 주차칸 번호
 * @param {long} fee - 계산된 주차 요금
 * @returns {void} 반환값 없음
 */
inline void showExitFee(LiquidCrystal_I2C& exitLcd, int slotId, long fee) {
  showLcdMessage(exitLcd, "Slot " + String(slotId) + " Exit", "Fee: " + String(fee));
}

#endif
