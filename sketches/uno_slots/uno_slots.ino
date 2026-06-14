const int SLOT_COUNT = 2;                    // 관리할 주차칸 개수
const unsigned long ECHO_TIMEOUT_US = 30000; // 초음파 응답 대기 최대 시간
const unsigned long SERIAL_LOG_INTERVAL_MS = 500; // 주차칸 상태 로그 출력 주기

const int SLOT_OCCUPIED_DISTANCE_CM[SLOT_COUNT] = {16, 10}; // 주차칸별 차량 감지 거리
const int SLOT_TRIG_PINS[SLOT_COUNT] = {4, 13};      // 주차칸별 초음파 송신 핀
const int SLOT_ECHO_PINS[SLOT_COUNT] = {5, 12};      // 주차칸별 초음파 수신 핀
const int SLOT_RED_LED_PINS[SLOT_COUNT] = {8, 10};   // 주차칸별 점유 표시 LED 핀
const int SLOT_GREEN_LED_PINS[SLOT_COUNT] = {9, 11}; // 주차칸별 빈자리 표시 LED 핀

bool previousOccupied[SLOT_COUNT] = {false, false}; // 주차칸 상태 변화 감지용 이전 상태
unsigned long lastSerialLoggedAt = 0;               // 마지막 주차칸 로그 출력 시각

/**
 * 지정한 주차칸의 초음파 거리를 센티미터 단위로 측정한다.
 * @param {int} slotIndex - 측정할 주차칸 배열 인덱스
 * @returns {float} 측정된 거리, 감지 실패 시 -1
 */
float readDistanceCm(int slotIndex) {
  digitalWrite(SLOT_TRIG_PINS[slotIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(SLOT_TRIG_PINS[slotIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(SLOT_TRIG_PINS[slotIndex], LOW);

  long duration = pulseIn(SLOT_ECHO_PINS[slotIndex], HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

/**
 * 측정 거리로 지정한 주차칸의 점유 상태를 판단한다.
 * @param {int} slotIndex - 확인할 주차칸 배열 인덱스
 * @param {float} distanceCm - 초음파센서 측정 거리
 * @returns {bool} 차량이 있으면 true, 아니면 false
 */
bool isSlotOccupied(int slotIndex, float distanceCm) {
  return distanceCm > 0 && distanceCm <= SLOT_OCCUPIED_DISTANCE_CM[slotIndex];
}

/**
 * 주차칸 LED를 점유 상태에 맞게 갱신한다.
 * @param {int} slotIndex - 갱신할 주차칸 배열 인덱스
 * @param {bool} occupied - 주차칸 점유 여부
 * @returns {void} 반환값 없음
 */
void updateSlotLed(int slotIndex, bool occupied) {
  digitalWrite(SLOT_RED_LED_PINS[slotIndex], occupied ? HIGH : LOW);
  digitalWrite(SLOT_GREEN_LED_PINS[slotIndex], occupied ? LOW : HIGH);
}

/**
 * Node.js 서버에 주차칸 상태 변화 이벤트를 전송한다.
 * @param {String} eventName - ENTRY 또는 VACATED 이벤트 이름
 * @param {int} slotId - 주차칸 번호
 * @returns {void} 반환값 없음
 */
void sendSlotEvent(String eventName, int slotId) {
  Serial.println(eventName + "," + String(slotId));
  Serial.print("Send event: ");
  Serial.print(eventName);
  Serial.print(",");
  Serial.println(slotId);
}

/**
 * 주차칸별 거리와 점유 상태를 시리얼 모니터에 출력한다.
 * @param {float[]} distances - 주차칸별 측정 거리 배열
 * @param {bool[]} occupiedSlots - 주차칸별 점유 상태 배열
 * @returns {void} 반환값 없음
 */
void logSlotStatus(float distances[], bool occupiedSlots[]) {
  if (millis() - lastSerialLoggedAt < SERIAL_LOG_INTERVAL_MS) {
    return;
  }

  lastSerialLoggedAt = millis();

  for (int index = 0; index < SLOT_COUNT; index++) {
    Serial.print("Slot ");
    Serial.print(index + 1);
    Serial.print(": ");

    if (distances[index] < 0) {
      Serial.print("no echo");
    } else {
      Serial.print(distances[index]);
      Serial.print(" cm, threshold ");
      Serial.print(SLOT_OCCUPIED_DISTANCE_CM[index]);
      Serial.print(" cm, ");
      Serial.print(occupiedSlots[index] ? "occupied" : "empty");
    }

    if (index < SLOT_COUNT - 1) {
      Serial.print(" | ");
    }
  }

  Serial.println();
}

/**
 * 각 주차칸의 점유 변화와 LED 표시를 처리한다.
 * @returns {void} 반환값 없음
 */
void updateParkingSlots() {
  float distances[SLOT_COUNT];
  bool occupiedSlots[SLOT_COUNT];

  for (int index = 0; index < SLOT_COUNT; index++) {
    distances[index] = readDistanceCm(index);
    occupiedSlots[index] = isSlotOccupied(index, distances[index]);

    bool occupied = occupiedSlots[index];
    int slotId = index + 1;

    updateSlotLed(index, occupied);

    if (occupied && !previousOccupied[index]) {
      sendSlotEvent("ENTRY", slotId);
    }

    if (!occupied && previousOccupied[index]) {
      sendSlotEvent("VACATED", slotId);
    }

    previousOccupied[index] = occupied;
  }

  logSlotStatus(distances, occupiedSlots);
}

/**
 * 주차칸 감지 보드의 현재 핀 설정을 시리얼 모니터에 출력한다.
 * @returns {void} 반환값 없음
 */
void printBootMessage() {
  Serial.println();
  Serial.println("=== Uno Slots Start ===");
  Serial.println("Baud: 9600");
  Serial.println("Slot 1: TRIG D4, ECHO D5, RED D8, GREEN D9, threshold 16 cm");
  Serial.println("Slot 2: TRIG D13, ECHO D12, RED D10, GREEN D11, threshold 10 cm");
}

/**
 * 주차칸 감지 보드를 초기화한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  delay(1000);
  printBootMessage();

  for (int index = 0; index < SLOT_COUNT; index++) {
    pinMode(SLOT_TRIG_PINS[index], OUTPUT);
    pinMode(SLOT_ECHO_PINS[index], INPUT);
    pinMode(SLOT_RED_LED_PINS[index], OUTPUT);
    pinMode(SLOT_GREEN_LED_PINS[index], OUTPUT);
  }
}

/**
 * 주차칸 상태를 반복 감지하고 Node.js 서버에 변화 이벤트를 보낸다.
 * @returns {void} 반환값 없음
 */
void loop() {
  updateParkingSlots();
  delay(300);
}
