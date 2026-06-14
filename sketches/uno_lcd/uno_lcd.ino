#include <LiquidCrystal_I2C.h>
#include "parking_lcd_display.h"

const int ENTRANCE_LCD_ADDRESS = 0x27; // 입구 LCD I2C 주소
const int EXIT_LCD_ADDRESS = 0x3F;     // 출구 LCD I2C 주소
const unsigned long TEMPORARY_MESSAGE_TIME_MS = 5000; // 안내 문구 유지 시간

LiquidCrystal_I2C entranceLcd(ENTRANCE_LCD_ADDRESS, 16, 2); // 입구 안내 LCD
LiquidCrystal_I2C exitLcd(EXIT_LCD_ADDRESS, 16, 2);         // 출구 안내 LCD

int lastOccupiedSlots = 0;             // 기본 화면 복귀에 사용할 마지막 점유 수
int lastTotalSlots = PARKING_TOTAL_SLOTS; // 기본 화면 복귀에 사용할 전체 주차칸 수
unsigned long temporaryMessageUntil = 0;  // 안내 문구 표시 종료 시각

/**
 * 쉼표로 분리된 명령에서 지정 위치의 값을 추출한다.
 * @param {String} line - Node.js 서버에서 받은 명령 문자열
 * @param {int} fieldIndex - 추출할 필드 위치
 * @returns {String} 추출한 필드 값
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
 * 기본 주차 현황 화면으로 되돌린다.
 * @returns {void} 반환값 없음
 */
void showDefaultStatus() {
  showParkingStatus(entranceLcd, exitLcd, lastOccupiedSlots, lastTotalSlots);
}

/**
 * 임시 안내 문구가 끝나는 시각을 갱신한다.
 * @returns {void} 반환값 없음
 */
void keepTemporaryMessage() {
  temporaryMessageUntil = millis() + TEMPORARY_MESSAGE_TIME_MS;
}

/**
 * Node.js 서버에서 받은 LCD 표시 명령을 처리한다.
 * @param {String} line - Node.js 서버가 전송한 명령 문자열
 * @returns {void} 반환값 없음
 */
void handleDisplayCommand(String line) {
  line.trim();

  if (line.startsWith("LCD_STATUS,")) {
    lastOccupiedSlots = getCsvField(line, 1).toInt();
    lastTotalSlots = getCsvField(line, 2).toInt();

    if (temporaryMessageUntil == 0) {
      showDefaultStatus();
    }

    return;
  }

  if (line.startsWith("LCD_ENTRANCE_WELCOME,")) {
    int emptySlots = getCsvField(line, 1).toInt();
    showEntranceWelcome(entranceLcd, emptySlots);
    keepTemporaryMessage();
    return;
  }

  if (line == "LCD_ENTRANCE_FULL") {
    showParkingFull(entranceLcd);
    keepTemporaryMessage();
    return;
  }

  if (line.startsWith("LCD_EXIT_FEE,")) {
    int slotId = getCsvField(line, 1).toInt();
    long fee = getCsvField(line, 2).toInt();
    showExitFee(exitLcd, slotId, fee);
    keepTemporaryMessage();
  }
}

/**
 * Serial로 들어온 LCD 표시 명령을 모두 읽는다.
 * @returns {void} 반환값 없음
 */
void readDisplayCommands() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleDisplayCommand(line);
  }
}

/**
 * 임시 안내 문구 표시 시간이 끝나면 기본 주차 현황 화면으로 복귀한다.
 * @returns {void} 반환값 없음
 */
void updateDefaultDisplay() {
  if (temporaryMessageUntil == 0 || millis() < temporaryMessageUntil) {
    return;
  }

  temporaryMessageUntil = 0;
  showDefaultStatus();
}

/**
 * LCD 전광판 보드를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);

  entranceLcd.init();
  entranceLcd.backlight();
  exitLcd.init();
  exitLcd.backlight();

  showParkingStatus(entranceLcd, exitLcd, 0, PARKING_TOTAL_SLOTS);
  Serial.println("=== Uno 3 LCD Display Start ===");
  Serial.println("Entrance LCD: 0x27, Exit LCD: 0x3F");
}

/**
 * Node.js 서버의 LCD 표시 명령을 반복 처리한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  readDisplayCommands();
  updateDefaultDisplay();
  delay(50);
}
