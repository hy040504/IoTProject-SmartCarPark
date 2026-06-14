#include <LiquidCrystal_I2C.h>
#include "parking_lcd_display.h"

const int ENTRANCE_LCD_ADDRESS = 0x27; // 입구 LCD I2C 주소
const int EXIT_LCD_ADDRESS = 0x3F;     // 출구 LCD I2C 주소
const unsigned long SCREEN_INTERVAL_MS = 2000; // 테스트 화면 전환 간격

LiquidCrystal_I2C entranceLcd(ENTRANCE_LCD_ADDRESS, 16, 2); // 입구 안내 LCD
LiquidCrystal_I2C exitLcd(EXIT_LCD_ADDRESS, 16, 2);         // 출구 안내 LCD

int screenIndex = 0;
unsigned long lastScreenChangedAt = 0;

/**
 * 현재 순서에 맞는 LCD 테스트 화면을 표시한다.
 * @param {int} index - 표시할 테스트 화면 번호
 * @returns {void} 반환값 없음
 */
void showTestScreen(int index) {
  if (index == 0) {
    showParkingStatus(entranceLcd, exitLcd, 1, PARKING_TOTAL_SLOTS);
    Serial.println("LCD test: status");
    return;
  }

  if (index == 1) {
    showEntranceWelcome(entranceLcd, 1);
    showLcdMessage(exitLcd, "Parking Status", "Cars: 1/2");
    Serial.println("LCD test: entrance welcome");
    return;
  }

  if (index == 2) {
    showParkingFull(entranceLcd);
    showLcdMessage(exitLcd, "Parking Status", "Cars: 2/2");
    Serial.println("LCD test: entrance full");
    return;
  }

  showLcdMessage(entranceLcd, "Parking Status", "Cars: 1/2");
  showExitFee(exitLcd, 2, 1500);
  Serial.println("LCD test: exit fee");
}

/**
 * LCD 2개 테스트 환경을 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  entranceLcd.init();
  entranceLcd.backlight();
  exitLcd.init();
  exitLcd.backlight();

  Serial.println("=== Uno 3 Dual LCD Test Start ===");
  Serial.println("Entrance LCD: 0x27, Exit LCD: 0x3F");
  showTestScreen(screenIndex);
}

/**
 * LCD 2개에 주요 화면을 순서대로 반복 표시한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  if (millis() - lastScreenChangedAt < SCREEN_INTERVAL_MS) {
    return;
  }

  lastScreenChangedAt = millis();
  screenIndex = (screenIndex + 1) % 4;
  showTestScreen(screenIndex);
}
