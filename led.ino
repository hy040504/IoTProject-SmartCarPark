/**
 * 루트 스케치가 잘못 업로드됐을 때 보드별 스케치 위치를 안내한다.
 * @returns {void} 반환값 없음
 */
void setup() {
  Serial.begin(9600);
  Serial.println("보드별 sketches 폴더의 .ino 파일을 업로드하세요.");
}

/**
 * 루트 스케치는 실제 주차장 제어를 수행하지 않는다.
 * @returns {void} 반환값 없음
 */
void loop() {
  delay(1000);
}
