#ifndef CONFIG_H
#define CONFIG_H

// ===================== 통신 설정 =================
#define COMM_BAUDRATE 57600

// ===================== 다이나믹셀 설정 =====================
#define DXL_BAUDRATE 1000000
#define DXL_PROTOCOL_VERSION 2.0

// 보드 방향 핀 값: 2
#define DXL_DIR_PIN 84     // OpenCR 방향 핀

#define LEFT_ID  1
#define RIGHT_ID 2

// ===================== 주행 파라미터 =====================
#define BASE_RPM 30.0f
#define MAX_RPM 60.0f
#define MIN_RPM -60.0f

#define TURN_GAIN_RPM_PER_DEG 0.6f
#define ANGLE_LIMIT_DEG 30.0f

#endif
