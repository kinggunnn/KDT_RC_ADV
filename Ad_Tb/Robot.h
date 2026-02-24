//====================================================
// 작업자 : 임진효
// 최신화 일자 : 2026_02_23
// 용도 : 프로젝트 공용 설정/타입/함수 선언
//====================================================

#ifndef ROBOT_H
#define ROBOT_H
#include <Arduino.h>
#include <stdint.h>

// -----------------------------
// [Dynamixel 설정]
// - UNO는 USB Serial(Serial)을 라즈베리파이 통신에 쓰므로
//   Dynamixel은 SoftwareSerial로 분리해서 사용한다.
// -----------------------------
#define DXL_DIR_PIN 2      // Dynamixel Half-duplex 방향 제어 핀(Shield 설정에 맞춰야 함)
#define DXL_RX_PIN  8      // UNO가 "받는" 핀 (SoftwareSerial RX)
#define DXL_TX_PIN  9      // UNO가 "보내는" 핀 (SoftwareSerial TX)

constexpr uint32_t PI_BAUD  = 115200;  // Pi ↔ UNO (USB Serial)
constexpr uint32_t DXL_BAUD = 57600;   // XL430 기본 baudrate(권장)

// XL430 ID (두 모터)
constexpr uint8_t DXL_ID_1 = 1;
constexpr uint8_t DXL_ID_2 = 2;

// -----------------------------
// [상태]
// - Pi가 0~9를 보내면, UNO는 이 값을 상태로 사용
// -----------------------------
enum class AiState : uint8_t {
  S0 = 0, S1, S2, S3, S4, S5, S6, S7, S8, S9
};

// -----------------------------
// [Dynamixel 제어 API] (DxlControl.cpp에서 구현)
// -----------------------------
bool dxlInit();
void dxlTorqueOnAll();
void dxlTorqueOffAll();

// 위치/속도 제어(필요한 것만 쓰면 됨)
void dxlSetPosition(uint8_t id, int32_t pos);     // pos: 0~4095 (모드/설정에 따라 다름)
void dxlSetVelocity(uint8_t id, int32_t vel);     // vel 단위는 DXL 스펙에 따름

// 상태값(0~9)에 따른 동작 매핑(ino에서 호출)
void applyAiState(AiState s);

// -----------------------------
// [Pi 통신 파서] (ino에서 사용)
// - Pi가 '0'~'9' 또는 "0\n" 같은 형태로 보낸다고 가정
// -----------------------------
bool readAiStateFromSerial(AiState &out);