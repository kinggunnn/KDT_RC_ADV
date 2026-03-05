#ifndef FUNC_H
#define FUNC_H

#include <Arduino.h>
#include <Dynamixel2Arduino.h>

// --- [통신 속도 및 포트 설정] ---
#define RPI_SERIAL_BAUD    115200
#define DXL_SERIAL_BAUD    1000000  // 1Mbps
#define DEBUG_LED 13

// --- [다이나믹셀 하드웨어 설정] ---
const int DXL_DIR_PIN = 2;
const uint8_t LEFT_ID = 1;
const uint8_t RIGHT_ID = 2;
const float DXL_PROTOCOL_VERSION = 2.0;

// --- [주행 파라미터 설정] ---
const float BASE_RPM = 30.0f;
const float MAX_RPM = 100.0f;
const float MIN_RPM = -100.0f;
const float TURN_GAIN = 0.6f;
const float SIDE_BIAS_RPM = 6.0f;
const float ANGLE_LIMIT_DEG = 30.0f;

// --- [상태 변수 및 객체 외부 선언] ---
extern Dynamixel2Arduino dxl;
extern bool driveEnabled;
extern float lastAngle;

// --- [클래스(동작 모드) 정의] ---
enum DriveClass {
    CLASS_NORMAL = 0, 
    CLASS_KEEP = 1, 
    CLASS_NO_LANE = 2,
    CLASS_PARKING = 3, 
    CLASS_HUMAN = 4, 
    CLASS_OBSTACLE = 5,
    CLASS_CROSSWALK = 6, 
    CLASS_BIAS_LEFT = 7, 
    CLASS_BIAS_RIGHT = 8,
    CLASS_START = 9, 
    CLASS_STOP = 10
};

// --- [함수 프로토타입] ---
void initializeDriveSystem();
void readCommandSerial();
void parseCommand(char* line);
void classAction(int classCode, float angleDeg);
void setWheelRPM(float leftRpm, float rightRpm);
void stopMotors();
void applyAngleDrive(float angleDeg, float speedScale, float extraBiasRpm);
void doParkingRoutine();
void doEvasiveManeuver();
float clampf(float value, float minimum, float maximum);

#endif