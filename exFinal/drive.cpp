#include <Arduino.h>
#include "drive.h"
#include "motor.h"

static unsigned long actionStartTime = 0;
static int currentAction = -1;
static int actionDuration = 0;

void processDrive(float angle, int action)
{
    unsigned long now = millis();

    // 새로운 action 들어오면 타이머 초기화
    if(action != currentAction)
    {
        currentAction = action;
        actionStartTime = now;

        switch(action)
        {
            case 1: 
                actionDuration = 2000; 
                break; // LEFT 2초
            case 2: 
                actionDuration = 2000; 
                break; // RIGHT 2초
            default: 
                actionDuration = 0; 
                break;
        }
    }

    // 시간이 끝났으면 정지(추후 수정. 직진이 나을지도?)
    if(actionDuration > 0 && now - actionStartTime > actionDuration)
    {
        stopMotors();
        return;
    }

    switch(currentAction)
    {
        case 0:
            applyAngleDrive(angle,1.0,0);
            break;

        case 1:
            applyAngleDrive(-30,1.0,0);
            break;

        case 2:
            applyAngleDrive(30,1.0,0);
            break;

        case 3:
            stopMotors();
            break;

        case 4:
            applyAngleDrive(angle,0.5,0);
            break;
    }
}