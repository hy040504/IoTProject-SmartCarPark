#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int LCD_ADDRESS_27 = 0x27;
const int LCD_ADDRESS_3F = 0x3F;
const int LCD_COLUMNS = 20;
const int LCD_ROWS = 4;
const unsigned long SCREEN_INTERVAL_MS = 2000;
const unsigned long SERIAL_ALIVE_INTERVAL_MS = 2000;

LiquidCrystal_I2C lcd27(LCD_ADDRESS_27, LCD_COLUMNS, LCD_ROWS);
LiquidCrystal_I2C lcd3f(LCD_ADDRESS_3F, LCD_COLUMNS, LCD_ROWS);
LiquidCrystal_I2C* activeLcd = &lcd3f;

int screenIndex = 0;
unsigned long nextScreenAt = 0;
unsigned long lastAliveAt = 0;
int activeLcdAddress = LCD_ADDRESS_3F;
bool lcdReady = false;

bool isI2cDevicePresent(int address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void selectDetectedLcd() {
  if (isI2cDevicePresent(LCD_ADDRESS_27)) {
    activeLcd = &lcd27;
    activeLcdAddress = LCD_ADDRESS_27;
    lcdReady = true;
    Serial.println("LCD FOUND 0x27");
    return;
  }

  if (isI2cDevicePresent(LCD_ADDRESS_3F)) {
    activeLcd = &lcd3f;
    activeLcdAddress = LCD_ADDRESS_3F;
    lcdReady = true;
    Serial.println("LCD FOUND 0x3F");
    return;
  }

  activeLcd = &lcd3f;
  activeLcdAddress = LCD_ADDRESS_3F;
  lcdReady = false;
  Serial.println("LCD NOT FOUND");
  Serial.println("CHECK SDA=A4 SCL=A5 VCC GND");
}

void printLine(int row, String text) {
  if (text.length() > LCD_COLUMNS) {
    text = text.substring(0, LCD_COLUMNS);
  }

  while (text.length() < LCD_COLUMNS) {
    text += " ";
  }

  activeLcd->setCursor(0, row);
  activeLcd->print(text);
}

void showScreen() {
  activeLcd->clear();

  if (screenIndex == 0) {
    printLine(0, "LCD Simple Test");
    printLine(1, "Uno3 I2C OK");
    printLine(2, "20x4 mode");
    printLine(3, "COM7 9600");
  } else if (screenIndex == 1) {
    printLine(0, "A4 SDA");
    printLine(1, "A5 SCL");
    printLine(2, "VCC 5V");
    printLine(3, "GND GND");
  } else {
    printLine(0, "Exit 14:30:25");
    printLine(1, "Slot 1 Fee 1500W");
    printLine(2, "Parked 1h 2m 5s");
    printLine(3, "Thank you. Bye!");
  }

  screenIndex = (screenIndex + 1) % 3;
  nextScreenAt = millis() + SCREEN_INTERVAL_MS;
}

void printAlive() {
  if (millis() - lastAliveAt < SERIAL_ALIVE_INTERVAL_MS) {
    return;
  }

  lastAliveAt = millis();
  Serial.print("UNO3 LCD ALIVE 0x");
  if (lcdReady) {
    Serial.println(activeLcdAddress, HEX);
    return;
  }

  Serial.println("NONE");
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("UNO3 LCD START");
  Serial.println("SDA=A4 SCL=A5");
  selectDetectedLcd();

  if (lcdReady) {
    activeLcd->init();
    activeLcd->backlight();
    showScreen();
  }
}

void loop() {
  printAlive();

  if (lcdReady && millis() >= nextScreenAt) {
    showScreen();
  }

  delay(50);
}
