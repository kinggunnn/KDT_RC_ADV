#ifndef FUNC_H
#define FUNC_H

#include <Arduino.h>

// clampf:
// - 값 범위 제한
float clampf(float value, float minimum, float maximum);

// setWheelRPM:
// - 최종적으로 다이나믹셀에 Goal Velocity를 쓰는 함수(핵심 출력)
void setWheelRPM(float leftRpm, float rightRpm);
void stopMotors();

// applyAngleDrive:
// - angle + speedScale -> left/right rpm 계산(차동구동 조향)
void applyAngleDrive(float angleDeg, float speedScale, float extraBiasRpm);

// doParkingRoutine / doEvasiveManeuver:
// - 특정 상황에 대한 "하드코딩된 시퀀스"(delay로 막는 블로킹)
void doParkingRoutine();
void doEvasiveManeuver();

// classAction:
// - class 번호에 따라 어떤 동작을 할지 결정하는 중심 함수
void classAction(int class_, float angleDeg);
void parseClass(char* line);

// setup()에서 호출하며 모터와 시리얼을 초기화한다.
void initializeDriveSystem();

// loop()에서 반복 호출하며 라즈베리파이 명령을 읽고 처리한다.
void readCommandSerial();

// 하드 좌회전
void turnLeft();

#endif
