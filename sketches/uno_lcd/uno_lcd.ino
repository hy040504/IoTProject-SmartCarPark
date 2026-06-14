#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "parking_lcd_display.h"

const int EXIT_LCD_ADDRESS_27 = 0x27;
const int EXIT_LCD_ADDRESS_3F = 0x3F;
const unsigned long TEMPORARY_MESSAGE_TIME_MS = 5000;
const unsigned long SERIAL_ALIVE_INTERVAL_MS = 3000;
const unsigned long DISPLAY_REFRESH_INTERVAL_MS = 1000;

LiquidCrystal_I2C exitLcd27(EXIT_LCD_ADDRESS_27, LCD_COLUMNS, LCD_ROWS);
LiquidCrystal_I2C exitLcd3f(EXIT_LCD_ADDRESS_3F, LCD_COLUMNS, LCD_ROWS);
LiquidCrystal_I2C* exitLcd = &exitLcd3f;

int lastOccupiedSlots = 0;
int lastTotalSlots = PARKING_TOTAL_SLOTS;
String lastStatusDate = "--";
String lastStatusTime = "--";
unsigned long temporaryMessageUntil = 0;
unsigned long lastAliveAt = 0;
unsigned long lastDisplayRefreshAt = 0;
int activeExitLcdAddress = EXIT_LCD_ADDRESS_3F;
bool lcdReady = false;

bool isI2cDevicePresent(int address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void selectExitLcd() {
  if (isI2cDevicePresent(EXIT_LCD_ADDRESS_27)) {
    exitLcd = &exitLcd27;
    activeExitLcdAddress = EXIT_LCD_ADDRESS_27;
    lcdReady = true;
    Serial.println("LCD FOUND 0x27");
    return;
  }

  if (isI2cDevicePresent(EXIT_LCD_ADDRESS_3F)) {
    exitLcd = &exitLcd3f;
    activeExitLcdAddress = EXIT_LCD_ADDRESS_3F;
    lcdReady = true;
    Serial.println("LCD FOUND 0x3F");
    return;
  }

  exitLcd = &exitLcd3f;
  activeExitLcdAddress = EXIT_LCD_ADDRESS_3F;
  lcdReady = false;
  Serial.println("LCD NOT FOUND");
  Serial.println("CHECK SDA=A4 SCL=A5 VCC GND");
}

/**
 * CSV에서 원하는 필드만 꺼낸다.
 * @param {String} line - 수신한 한 줄
 * @param {int} fieldIndex - 추출할 필드 번호
 * @returns {String} 추출된 필드 문자열
 */
String getCsvField(String line, int fieldIndex) {
  int currentField = 0;
  int startIndex = 0;

  for (int index = 0; index <= line.length(); index++) {
    if (index == line.length() || line.charAt(index) == ',') {
      if (currentField == fieldIndex) {
        return line.substring(startIndex, index);
      }

      currentField++;
      startIndex = index + 1;
    }
  }

  return "";
}

/**
 * 기본 상태를 출구 LCD에 다시 그린다.
 * @returns {void} 반환값 없음
 */
void showDefaultStatus() {
  if (!lcdReady) {
    return;
  }

  showLcdMessage(
    *exitLcd,
    "Date " + lastStatusDate,
    "Time " + lastStatusTime,
    "Smart Parking Team",
    "Cars: " + String(lastOccupiedSlots) + "/" + String(lastTotalSlots) + " | YSH JKH"
  );
}

/**
 * 잠깐 보여줄 메시지의 종료 시각을 갱신한다.
 * @returns {void} 반환값 없음
 */
void rememberTemporaryMessage() {
  temporaryMessageUntil = millis() + TEMPORARY_MESSAGE_TIME_MS;
}

/**
 * 임시 경고 메시지를 지정 시간 동안 표시한다.
 * @param {String} firstLine - 첫 번째 줄
 * @param {String} secondLine - 두 번째 줄
 * @param {String} thirdLine - 세 번째 줄
 * @param {String} fourthLine - 네 번째 줄
 * @param {unsigned long} durationMs - 표시 유지 시간
 * @returns {void} 반환값 없음
 */
void showTemporaryMessage(
  const String& firstLine,
  const String& secondLine,
  const String& thirdLine,
  const String& fourthLine,
  unsigned long durationMs
) {
  if (!lcdReady) {
    return;
  }

  showLcdMessage(*exitLcd, firstLine, secondLine, thirdLine, fourthLine);
  temporaryMessageUntil = millis() + durationMs;
}

/**
 * 서버가 보낸 LCD 명령을 처리한다.
 * @param {String} line - 수신한 한 줄
 * @returns {void} 반환값 없음
 */
void handleDisplayCommand(String line) {
  line.trim();

  if (line.startsWith("LCD_STATUS,")) {
    lastOccupiedSlots = getCsvField(line, 1).toInt();
    lastTotalSlots = getCsvField(line, 2).toInt();
    lastStatusDate = getCsvField(line, 3);
    lastStatusTime = getCsvField(line, 4);

    if (temporaryMessageUntil == 0) {
      showDefaultStatus();
    }

    return;
  }

  if (line.startsWith("LCD_EXIT_FEE,")) {
    int slotId = getCsvField(line, 1).toInt();
    long fee = getCsvField(line, 2).toInt();
    unsigned long parkedSeconds = getCsvField(line, 3).toInt();
    String exitTime = getCsvField(line, 4);

    if (exitTime.length() == 0) {
      exitTime = "Now";
    }

    if (lcdReady) {
      showExitFee(*exitLcd, slotId, fee, exitTime, parkedSeconds);
    }

    rememberTemporaryMessage();
    return;
  }

  if (line.startsWith("LCD_FULL_WARNING,")) {
    unsigned long durationMs = getCsvField(line, 1).toInt();
    int occupiedSlots = getCsvField(line, 2).toInt();
    int totalSlots = getCsvField(line, 3).toInt();

    if (durationMs == 0) {
      durationMs = 4000;
    }

    if (occupiedSlots <= 0) {
      occupiedSlots = lastTotalSlots;
    }

    if (totalSlots <= 0) {
      totalSlots = lastTotalSlots;
    }

    showTemporaryMessage(
      "Parking Full",
      "Cannot Enter",
      "Please Wait",
      "Cars: " + String(occupiedSlots) + "/" + String(totalSlots),
      durationMs
    );
  }
}

/**
 * 시리얼 버퍼에서 LCD 명령을 읽는다.
 * @returns {void} 반환값 없음
 */
void readDisplayCommands() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleDisplayCommand(line);
  }
}

/**
 * 임시 메시지 시간이 끝나면 기본 상태로 되돌린다.
 * @returns {void} 반환값 없음
 */
void updateDefaultDisplay() {
  if (temporaryMessageUntil == 0) {
    return;
  }

  if (millis() < temporaryMessageUntil) {
    return;
  }

  temporaryMessageUntil = 0;
  showDefaultStatus();
}

void printAlive() {
  if (millis() - lastAliveAt < SERIAL_ALIVE_INTERVAL_MS) {
    return;
  }

  lastAliveAt = millis();
  Serial.print("UNO3 LCD ALIVE 0x");
  if (lcdReady) {
    Serial.println(activeExitLcdAddress, HEX);
    return;
  }

  Serial.println("NONE");
}

void refreshStatusDisplayIfNeeded() {
  if (!lcdReady || temporaryMessageUntil != 0) {
    return;
  }

  if (millis() - lastDisplayRefreshAt < DISPLAY_REFRESH_INTERVAL_MS) {
    return;
  }

  lastDisplayRefreshAt = millis();
  showDefaultStatus();
}

/**
 * 출구 LCD만 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("UNO3 LCD START");
  Serial.println("SDA=A4 SCL=A5");
  selectExitLcd();

  if (lcdReady) {
    exitLcd->init();
    exitLcd->backlight();
    showDefaultStatus();
  }
}

/**
 * 서버가 보낸 명령을 계속 반영한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  printAlive();
  readDisplayCommands();
  refreshStatusDisplayIfNeeded();
  updateDefaultDisplay();
  delay(50);
}
