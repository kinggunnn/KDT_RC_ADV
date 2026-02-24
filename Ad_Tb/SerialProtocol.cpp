//====================================================
// 작업자 : 임진효
// 최신화 일자 : 2026_02_23
// 용도 : Pi(USB Serial)로부터 AI 결과(0~9) 수신 파서
//====================================================

#include "Robot.h"

/*
  =====================================================
  readAiStateFromSerial()
  -----------------------------------------------------
  역할:
    - Serial에서 '0'~'9' 또는 "0\n" 같은 데이터를 받아 AiState로 변환
    - 줄바꿈(\n, \r)은 무시
  반환:
    true  : out에 새 상태가 들어감
    false : 아직 유효 데이터 없음
  =====================================================
*/
bool readAiStateFromSerial(AiState &out) {
  while (Serial.available()) {
    char c = (char)Serial.read();

    // 줄바꿈 무시
    if (c == '\n' || c == '\r' || c == ' ') continue;

    if (c >= '0' && c <= '9') {
      out = (AiState)(c - '0');
      return true;
    }
  }
  return false;
}