#ifndef FEE_REQUEST_H
#define FEE_REQUEST_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

const char WIFI_SSID[] = "iptime";                       // Wemos가 접속할 Wi-Fi 이름
const char WIFI_PASSWORD[] = "00000000";                 // Wemos가 접속할 Wi-Fi 비밀번호
const char FEE_SERVER_BASE_URL[] = "http://10.27.17.73:3000"; // 이전 Wi-Fi 구조에서 사용할 요금 서버 주소
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;     // Wi-Fi 연결 대기 최대 시간

/**
 * 요금 서버 호출을 위해 Wemos를 Wi-Fi에 연결한다.
 * @returns {bool} Wi-Fi 연결 성공 여부
 */
inline bool setupParkingFeeClient() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startedAt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS) {
    delay(300);
  }

  return WiFi.status() == WL_CONNECTED;
}

/**
 * 요금 서버의 GET URL을 호출하고 응답 본문을 반환한다.
 * @param {String} path - 호출할 서버 경로
 * @returns {String} 서버 응답 본문, 실패 시 빈 문자열
 */
inline String requestFeeServer(String path) {
  if (WiFi.status() != WL_CONNECTED && !setupParkingFeeClient()) {
    return "";
  }

  WiFiClient client;
  HTTPClient http;
  String url = String(FEE_SERVER_BASE_URL) + path;

  if (!http.begin(client, url)) {
    return "";
  }

  int statusCode = http.GET();
  String response = statusCode >= 200 && statusCode < 300 ? http.getString() : "";

  http.end();
  return response;
}

/**
 * 차량 입차 이벤트를 요금 서버에 GET 요청으로 전송한다.
 * @param {int} slotId - 입차한 주차칸 번호
 * @returns {bool} 서버 요청 성공 여부
 */
inline bool sendVehicleEntryRequest(int slotId) {
  String response = requestFeeServer("/parking/entry?slot=" + String(slotId));
  return response.length() > 0;
}

/**
 * 차량 출차 이벤트를 요금 서버에 GET 요청으로 전송한다.
 * @param {int} slotId - 출차한 주차칸 번호
 * @returns {String} 서버에서 반환한 요금 정보 JSON
 */
inline String sendVehicleExitRequest(int slotId) {
  return requestFeeServer("/parking/exit?slot=" + String(slotId));
}

/**
 * 서버 JSON 응답에서 요금 숫자만 간단히 추출한다.
 * @param {String} response - 요금 서버 JSON 응답
 * @returns {long} 추출한 요금, 실패 시 -1
 */
inline long extractFeeFromResponse(String response) {
  int keyIndex = response.indexOf("\"fee\":");

  if (keyIndex < 0) {
    return -1;
  }

  int valueStart = keyIndex + 6;
  int valueEnd = response.indexOf('}', valueStart);

  if (valueEnd < 0) {
    valueEnd = response.length();
  }

  return response.substring(valueStart, valueEnd).toInt();
}

#endif
