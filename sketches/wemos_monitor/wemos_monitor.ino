#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

const char WIFI_SSID[] = "iptime";                     // 접속할 Wi-Fi 이름
const char WIFI_PASSWORD[] = "00000000";               // 접속할 Wi-Fi 비밀번호
const char FEE_SERVER_SESSIONS_URL[] = "http://10.27.17.73:3000/parking/sessions";  // 세션 조회 API 주소
const unsigned long POLL_INTERVAL_MS = 5000;           // 서버 상태 조회 주기

unsigned long lastPolledAt = 0; // 마지막 서버 조회 시각

/**
 * Wi-Fi 연결을 초기화한다.
 * @returns {bool} Wi-Fi 연결 성공 여부
 */
bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startedAt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 10000) {
    delay(300);
  }

  return WiFi.status() == WL_CONNECTED;
}

/**
 * 요금 서버의 현재 주차 세션 목록을 요청한다.
 * @returns {String} 서버 응답 본문, 실패 시 빈 문자열
 */
String requestSessions() {
  if (WiFi.status() != WL_CONNECTED && !connectWifi()) {
    return "";
  }

  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, FEE_SERVER_SESSIONS_URL)) {
    return "";
  }

  int statusCode = http.GET();
  String response = statusCode >= 200 && statusCode < 300 ? http.getString() : "";

  http.end();
  return response;
}

/**
 * 모니터링용 Wemos 보드를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(115200);
  connectWifi();
}

/**
 * 요금 서버 상태를 주기적으로 시리얼 모니터에 출력한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  if (millis() - lastPolledAt < POLL_INTERVAL_MS) {
    return;
  }

  lastPolledAt = millis();
  Serial.println(requestSessions());
}
