#include "communication.h"
#include "drive.h"
#include "motor.h"
#include "state_manager.h"

void setup()
{
    initCommunication();
    initMotor();

    driveEnabled = true;
}

void loop()
{
    if (readCommand(class_, angle, action))
    {
        lastClass = class_;
        lastAngle = angle;
        lastAction = action;
    }

    updateHeldDriveAngle(lastClass, lastAngle);
    updateEmergencyState(lastClass);

    if (emergencyLatched)
        return;

    if (waitingExitSignal && !isRoutineActive())
    {
        if (lastClass == 9)
        {
            waitingExitSignal = false;
            activeMission = MISSION_NONE;
            startRoutine(2);
        }

        processRoutine();
        updateMissionRoutineState();
        return;
    }

    if (handleActionSequence(lastClass, lastAngle, lastAction))
    {
        processRoutine();
        updateMissionRoutineState();
        return;
    }

    switch (lastClass)
    {
        case 1:
            if (driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;

        case 2:
        case 5:
            if (driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;

        case 6:
        case 8:
            if (driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_LEFT);
            break;

        case 7:
            if (driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;

        case 0:
            if (driveEnabled && !isRoutineActive())
                processDrive(heldDriveAngle, ACT_FORWARD);
            break;

        case 9:
            break;

        case 3:
        case 4:
            emergencyStop();
            break;

        default:
            if (driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;
    }

    processRoutine();
    updateMissionRoutineState();
}