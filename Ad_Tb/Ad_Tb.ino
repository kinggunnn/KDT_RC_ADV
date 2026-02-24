//====================================================
// 작업자 : 임진효
// 최신화 일자 : 2026_02_23
// 용도 : 메인 구동부(상태 수신 → 동작 실행)
//====================================================

#include "Robot.h"

// 현재 상태
static AiState g_state = AiState::S0;

// -----------------------------------------------------
// setup()
// -----------------------------------------------------
void setup() {
  Serial.begin(PI_BAUD);
  Serial.println("BOOT OK (UNO)");

  // Dynamixel 초기화
  if (!dxlInit()) {
    Serial.println("DXL INIT FAIL");
  } else {
    Serial.println("DXL INIT OK");
  }

  // 시작 상태 적용
  applyAiState(g_state);
}

// -----------------------------------------------------
// loop()
// - Pi로부터 상태(0~9)를 수신하면 즉시 applyAiState로 반영
// -----------------------------------------------------
void loop() {
  AiState s;
  if (readAiStateFromSerial(s)) {
    g_state = s;

    Serial.print("AI STATE=");
    Serial.println((uint8_t)g_state);

    applyAiState(g_state);
  }

  // 필요하면 여기에 "주기 동작", "타임아웃", "안전정지" 같은 로직 추가
}