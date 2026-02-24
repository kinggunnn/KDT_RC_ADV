//====================================================
// 작업자 : 임진효
// 최신화 일자 : 2026_02_23
// 용도 : Dynamixel(XL430) 제어 구현부
//====================================================

#include "Robot.h"
#include <SoftwareSerial.h>
#include <Dynamixel2Arduino.h>

// -----------------------------
// [통신 객체]
// - SoftwareSerial: UNO의 일반 핀으로 UART 흉내
// - Dynamixel2Arduino: Protocol 2.0 제어
// -----------------------------
static SoftwareSerial dxlSerial(DXL_RX_PIN, DXL_TX_PIN); // RX, TX
static Dynamixel2Arduino dxl(dxlSerial, DXL_DIR_PIN);

static bool g_dxlReady = false;

bool dxlInit() {
  dxlSerial.begin(DXL_BAUD);

  dxl.begin(DXL_BAUD);
  dxl.setPortProtocolVersion(2.0);

  // 여기서 ID가 살아있는지 ping 체크해도 됨(선택)
  // if (!dxl.ping(DXL_ID_1)) return false;

  // 기본은 안전하게 토크 오프로 시작
  dxl.torqueOff(DXL_ID_1);
  dxl.torqueOff(DXL_ID_2);

  // 운영모드 설정(프로젝트 목적에 맞게 고정)
  // 위치제어 예시:
  dxl.setOperatingMode(DXL_ID_1, OP_POSITION);
  dxl.setOperatingMode(DXL_ID_2, OP_POSITION);

  dxl.torqueOn(DXL_ID_1);
  dxl.torqueOn(DXL_ID_2);

  g_dxlReady = true;
  return true;
}

void dxlTorqueOnAll() {
  if (!g_dxlReady) return;
  dxl.torqueOn(DXL_ID_1);
  dxl.torqueOn(DXL_ID_2);
}

void dxlTorqueOffAll() {
  if (!g_dxlReady) return;
  dxl.torqueOff(DXL_ID_1);
  dxl.torqueOff(DXL_ID_2);
}

void dxlSetPosition(uint8_t id, int32_t pos) {
  if (!g_dxlReady) return;
  // XL430 기본 분해능(0~4095) 기준으로 작성
  pos = constrain(pos, 0, 4095);
  dxl.setGoalPosition(id, pos);
}

void dxlSetVelocity(uint8_t id, int32_t vel) {
  if (!g_dxlReady) return;
  // 속도 모드를 쓸 거면 운영모드를 OP_VELOCITY로 바꿔야 함
  dxl.setGoalVelocity(id, vel);
}

/*
  =====================================================
  applyAiState()
  -----------------------------------------------------
  역할:
    - AiState(0~9) 값에 따라 모터 2개를 어떻게 움직일지 매핑
    - 지금은 "뼈대"만 제공(너희가 동작 정의 채우면 됨)

  주의:
    - 위치제어(OP_POSITION) 기준 예시
    - 속도제어로 할 거면 운영모드 변경 후 dxlSetVelocity 사용
  =====================================================
*/
void applyAiState(AiState s) {
  if (!g_dxlReady) return;

  switch (s) {
    case AiState::S0:
      // 예: 중립 위치
      dxlSetPosition(DXL_ID_1, 2048);
      dxlSetPosition(DXL_ID_2, 2048);
      break;

    case AiState::S1:
      // 예: 모터1 +, 모터2 -
      dxlSetPosition(DXL_ID_1, 1024);
      dxlSetPosition(DXL_ID_2, 3072);
      break;

    case AiState::S2:
      dxlSetPosition(DXL_ID_1, 3072);
      dxlSetPosition(DXL_ID_2, 1024);
      break;

    // TODO: S3~S9는 팀에서 정의
    default:
      break;
  }
}