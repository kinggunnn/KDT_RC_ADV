#include "communication.h"
#include "drive.h"
#include "motor.h"

int class_;
float angle;
int action;

int lastClass = 0;

bool driveEnabled = false;

void setup()
{
    initCommunication();
    initMotor();
}

void loop()
{
    if(readCommand(class_, angle, action))
    {
        lastClass = class_;
    }

    switch(lastClass)
    {
        case 9: // START
            driveEnabled = true;
            break;

        case 10: // ARRIVE
            stopMotors();
            driveEnabled = false;
            cancelRoutine();
            break;

        case 1: // 일반 주행
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, ACT_FORWARD);
            break;

        case 2: // 물류 루틴
            if(driveEnabled && !isRoutineActive())
                startRoutine(1);
            break;

        case 3: // 사람 감지
            stopMotors();
            driveEnabled = false;
            cancelRoutine();
            break;

        case 4: // 자동차 감지
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, ACT_SLOW);
            break;

        case 5: // 좌회피
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, ACT_LEFT);
            break;

        case 6: // 도착 주차
            if(driveEnabled && !isRoutineActive())
                startRoutine(2);
            break;

        default:
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, action);
            break;
    }

    processRoutine();
}