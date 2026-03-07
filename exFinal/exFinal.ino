#include "communication.h"
#include "drive.h"
#include "motor.h"

int class_;
float angle;
int action;

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
        switch(class_)
        {
            case 9: // START
                driveEnabled = true;
                applyAngleDrive(0,1.0,0); // 직진!!!!!!!!!!!!!
                break;

            case 10: // ARRIVE
                driveEnabled = false;
                stopMotors();
                break;

            case 1:
                processDrive(angle, ACT_FORWARD); // 직진!!!!!!!
                break;

            case 2: // 물류
                startRoutine(1);
                break;

            case 3: // 사람 감지
                stopMotors();
                break;

            case 4: // 자동차 감지
                if(driveEnabled && !isRoutineActive())
                    processDrive(angle, ACT_SLOW);
                break;

            case 5: 
                processDrive(angle, ACT_LEFT);
                break;

            case 6: // 도착 주차
                startRoutine(2);
                break;

            default:
                if(driveEnabled && !isRoutineActive())
                    processDrive(angle, action);
                break;
        }
    }

    processRoutine();
}