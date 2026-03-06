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
        // 라이다 클래스로 두는지 클래스 아이디로 둘지
        switch(class_)
        {
            case 9: // START
                driveEnabled = true;

                applyAngleDrive(0,1.0,0); // 직진
                delay(500);               // 0.5초
                stopMotors();

                break;

            case 10: // STOP
                driveEnabled = false;
                stopMotors();
                break;

            case 4: // PERSON
                stopMotors();
                break;

            case 5: // CAR
                processDrive(angle,4);
                break;

            default:
                if(driveEnabled)
                    processDrive(angle,action);
                break;
        }
    }
}