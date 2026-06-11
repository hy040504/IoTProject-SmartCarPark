#include <Wire.h>

/**
 * I2C 주소 스캔 결과를 출력할 시리얼 통신을 시작한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(1000);

  Serial.println();
  Serial.println("=== I2C Scanner Start ===");
  Serial.println("LCD SDA -> A4, SCL -> A5");
}

/**
 * I2C 장치 주소를 반복 스캔한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  int deviceCount = 0;

  Serial.println("Scanning...");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found");
  }

  delay(3000);
}
