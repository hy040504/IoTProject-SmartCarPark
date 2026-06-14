#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

const char WIFI_SSID[] = "iptime";                         // 접속할 Wi-Fi 이름
const char WIFI_PASSWORD[] = "00000000";                   // 접속할 Wi-Fi 비밀번호
const char FEE_SERVER_BASE_URL[] = "http://10.27.17.73:3000";  // 요금 서버 기본 주소

const int GATE_RX_PIN = D5;       // 차단기 Uno 송신선을 받는 핀
const int GATE_TX_PIN = D6;       // 차단기 Uno 수신선으로 보내는 핀
const int SLOT_RX_PIN = D7;       // 주차칸 Uno 송신선을 받는 핀
const int SLOT_TX_PIN = D8;       // 주차칸 Uno 수신선으로 보내는 핀
const int SLOT_COUNT = 2;         // 관리할 주차칸 개수

SoftwareSerial gateSerial(GATE_RX_PIN, GATE_TX_PIN); // 차단기 Uno 통신 포트
SoftwareSerial slotSerial(SLOT_RX_PIN, SLOT_TX_PIN); // 주차칸 Uno 통신 포트

bool slotOccupied[SLOT_COUNT] = {false, false}; // 서버 전송 기준 주차칸 점유 상태
int pendingExitSlotId = 0;                      // 차단기 통과를 기다리는 출차 칸 번호

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
 * 요금 서버 GET API를 호출한다.
 * @param {String} path - 호출할 API 경로
 * @returns {String} 서버 응답 본문, 실패 시 빈 문자열
 */
String requestFeeServer(String path) {
  if (WiFi.status() != WL_CONNECTED && !connectWifi()) {
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
 * 서버 JSON 응답에서 요금 숫자를 추출한다.
 * @param {String} response - 서버 JSON 응답
 * @returns {long} 추출한 요금, 실패 시 -1
 */
long extractFee(String response) {
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

/**
 * 현재 빈자리 개수를 계산한다.
 * @returns {int} 빈 주차칸 개수
 */
int countEmptySlots() {
  int emptySlots = 0;

  for (int index = 0; index < SLOT_COUNT; index++) {
    if (!slotOccupied[index]) {
      emptySlots++;
    }
  }

  return emptySlots;
}

/**
 * 입구/차단기 Uno에 빈자리 개수를 전송한다.
 * @returns {void} 반환값 없음
 */
void sendEmptySlotsToGate() {
  gateSerial.println("EMPTY," + String(countEmptySlots()));
}

/**
 * 주차칸 Uno가 보낸 이벤트를 처리한다.
 * @param {String} line - 주차칸 Uno 이벤트 문자열
 * @returns {void} 반환값 없음
 */
void handleSlotLine(String line) {
  line.trim();

  int commaIndex = line.indexOf(',');
  if (commaIndex < 0) {
    return;
  }

  String eventName = line.substring(0, commaIndex);
  int slotId = line.substring(commaIndex + 1).toInt();

  if (slotId < 1 || slotId > SLOT_COUNT) {
    return;
  }

  int slotIndex = slotId - 1;

  if (eventName == "ENTRY") {
    slotOccupied[slotIndex] = true;
    requestFeeServer("/parking/entry?slot=" + String(slotId));
    sendEmptySlotsToGate();
  }

  if (eventName == "VACATED") {
    slotOccupied[slotIndex] = false;
    pendingExitSlotId = slotId;
    sendEmptySlotsToGate();
  }
}

/**
 * 차단기 Uno가 보낸 이벤트를 처리한다.
 * @param {String} line - 차단기 Uno 이벤트 문자열
 * @returns {void} 반환값 없음
 */
void handleGateLine(String line) {
  line.trim();

  if (line != "BARRIER_EXIT" || pendingExitSlotId == 0) {
    return;
  }

  int slotId = pendingExitSlotId;
  pendingExitSlotId = 0;

  String response = requestFeeServer("/parking/exit?slot=" + String(slotId));
  long fee = extractFee(response);

  if (fee >= 0) {
    gateSerial.println("FEE," + String(slotId) + "," + String(fee));
  }
}

/**
 * 지정한 SoftwareSerial에서 수신된 한 줄 메시지를 읽는다.
 * @param {SoftwareSerial&} serialPort - 읽을 SoftwareSerial 포트
 * @returns {String} 수신한 한 줄 메시지, 없으면 빈 문자열
 */
String readLineFrom(SoftwareSerial& serialPort) {
  if (!serialPort.available()) {
    return "";
  }

  return serialPort.readStringUntil('\n');
}

/**
 * Wemos 요금 게이트웨이를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(115200);
  gateSerial.begin(9600);
  slotSerial.begin(9600);
  connectWifi();
  sendEmptySlotsToGate();
}

/**
 * Uno 보드 이벤트를 받아 요금 서버와 중계한다.
 * @returns {void} 반환값 없음
 */
void loop() {
  String slotLine = readLineFrom(slotSerial);
  if (slotLine.length() > 0) {
    handleSlotLine(slotLine);
  }

  String gateLine = readLineFrom(gateSerial);
  if (gateLine.length() > 0) {
    handleGateLine(gateLine);
  }

  delay(50);
}
