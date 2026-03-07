#include <Arduino.h>
#include "drive.h"
#include "motor.h"

/* ================= Timed Action ================= */
struct TimedAction
{
    BaseAction action;
    float angle;
    unsigned long duration;
};

/* ================= 기본 주행 실행 ================= */
void executeBaseAction(BaseAction act, float angle)
{
    switch(act)
    {
        case ACT_FORWARD:
            applyAngleDrive(angle,1.0,0);
            break;

        case ACT_LEFT:
            applyAngleDrive(-30,1.0,0);
            break;

        case ACT_RIGHT:
            applyAngleDrive(30,1.0,0);
            break;

        case ACT_ROTATE:
            setWheelRPM(20,-20);
            break;

        case ACT_REVERSE:
            setWheelRPM(-20,-20);
            break;

        case ACT_STOP:
            stopMotors();
            break;

        case ACT_SLOW:
            applyAngleDrive(angle,0.5,0);
            break;
    }
}

/* ================= 일반 주행 ================= */
static BaseAction currentAction = ACT_STOP;
static unsigned long actionStart = 0;
static unsigned long actionDuration = 0;

void processDrive(float angle, int action)
{
    if(routineActive) return;

    unsigned long now = millis();

    if(action != currentAction)
    {
        currentAction = (BaseAction)action;
        actionStart = now;

        switch(action)
        {
            case ACT_LEFT:
            case ACT_RIGHT:
                actionDuration = 2000;
                break;

            default:
                actionDuration = 0;
        }
    }

    if(actionDuration > 0 && now - actionStart > actionDuration)
    {
        currentAction = ACT_FORWARD;
        actionDuration = 0;
    }

    executeBaseAction(currentAction, angle);
}

/* ================= Routine ================= */
static bool routineActive = false;
static int routineIndex = 0;
static unsigned long routineStart = 0;

static int routineLength = 0;   // 추가

TimedAction logisticsRoutine[] =
{
    {ACT_FORWARD,0,2000},
    {ACT_REVERSE,0,1000},
    {ACT_STOP,0,0}
};

TimedAction finishRoutine[] =
{
    {ACT_RIGHT,0,2000},
    {ACT_FORWARD,0,1000},
    {ACT_STOP,0,0}
};

TimedAction* currentRoutine = nullptr;

bool isRoutineActive()
{
    return routineActive;
}

void cancelRoutine()
{
    routineActive = false;
}

/* ================= Routine 시작 ================= */
void startRoutine(int routine)
{
    routineActive = true;
    routineIndex = 0;
    routineStart = millis();

    actionDuration = 0;
    currentAction = ACT_STOP;

    switch(routine)
    {
        case 1:
            currentRoutine = logisticsRoutine;
            routineLength = sizeof(logisticsRoutine) / sizeof(TimedAction);
            break;

        case 2:
            currentRoutine = finishRoutine;
            routineLength = sizeof(finishRoutine) / sizeof(TimedAction);
            break;
    }
}

/* ================= Routine 실행 ================= */
void processRoutine()
{
    if(!routineActive || currentRoutine == nullptr)
        return;

    unsigned long now = millis();

    TimedAction &act = currentRoutine[routineIndex];

    executeBaseAction(act.action, act.angle);

    // duration 0이면 루틴 종료
    if(act.duration == 0)
    {
        routineActive = false;
        return;
    }

    if(now - routineStart >= act.duration)
    {
        routineIndex++;
        routineStart = now;

        // 루틴 끝 체크 (배열 범위 보호)
        if(routineIndex >= routineLength)
        {
            executeBaseAction(ACT_STOP,0);
            routineActive = false;
            return;
        }
    }
}