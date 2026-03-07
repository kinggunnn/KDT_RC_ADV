#ifndef DRIVE_H
#define DRIVE_H

#include "motor.h"

/* ================= 기본 액션 정의 ================= */
enum BaseAction
{
    ACT_FORWARD, // 직진 : 0
    ACT_LEFT, // 좌회전 : 1
    ACT_RIGHT, // 우회전 : 2
    ACT_ROTATE, // 회전 : 3
    ACT_STOP, // 정지 : 4
    ACT_SLOW, // 서행 : 5
    ACT_REVERSE // 후진 : 6
};


bool isRoutineActive();

// 일반 주행
void processDrive(float angle, int action);
void executeBaseAction(BaseAction act, float angle);

// 특수 주행
void startRoutine(int routine);
void processRoutine();

#endif