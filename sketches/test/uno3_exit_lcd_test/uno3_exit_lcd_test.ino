#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "parking_lcd_display.h"

const int EXIT_LCD_ADDRESS = 0x27;
const unsigned long STATUS_SHOW_TIME_MS = 4000;
const unsigned long FEE_SHOW_TIME_MS = 7000;

LiquidCrystal_I2C exitLcd(EXIT_LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

unsigned long nextChangeAt = 0;
bool showingFee = false;

/**
 * 테스트용 기본 상태를 표시한다.
 * @returns {void} 반환값 없음
 */
void showStatusScreen() {
  showLcdMessage(
    exitLcd,
    "Date 2026-06-14",
    "Time 16:48:12",
    "Smart Parking Team",
    "Cars: 1/2 | YSH JKH"
  );
  Serial.println("Exit LCD status screen");
}

/**
 * 테스트용 요금 화면을 표시한다.
 * @returns {void} 반환값 없음
 */
void showFeeScreen() {
  showExitFee(exitLcd, 1, 1500, "14:30:25", 3725);
  Serial.println("Exit LCD fee screen");
}

/**
 * LCD 전환 시간을 갱신한다.
 * @param {unsigned long} durationMs - 유지할 시간
 * @returns {void} 반환값 없음
 */
void scheduleNextChange(unsigned long durationMs) {
  nextChangeAt = millis() + durationMs;
}

/**
 * 출력 화면을 바꾼다.
 * @returns {void} 반환값 없음
 */
void advanceScreen() {
  showingFee = !showingFee;

  if (showingFee) {
    showFeeScreen();
    scheduleNextChange(FEE_SHOW_TIME_MS);
    return;
  }

  showStatusScreen();
  scheduleNextChange(STATUS_SHOW_TIME_MS);
}

/**
 * 시리얼 안내를 출력한다.
 * @returns {void} 반환값 없음
 */
void printBanner() {
  Serial.println("=== Uno 3 Exit LCD Test Start ===");
  Serial.println("LCD: 0x27");
  Serial.println("Status and fee screens alternate every 7 seconds.");
}

/**
 * 테스트용 LCD를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  exitLcd.init();
  exitLcd.backlight();
  printBanner();
  showStatusScreen();
  scheduleNextChange(STATUS_SHOW_TIME_MS);
}

/**
 * LCD 화면을 순환한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  if (millis() >= nextChangeAt) {
    advanceScreen();
  }

  delay(50);
}
