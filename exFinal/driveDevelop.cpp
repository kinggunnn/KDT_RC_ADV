#include <Arduino.h>
#include "drive.h"
#include "motor.h"

/*
 * 주행 로직 개발/실험용 파일
 * (모드 기반 분기 방식 참고용)
 */

enum DriveMode
{
    MODE_MANUAL,
    MODE_ROUTINE
};

// 현재 주행 모드(수동/루틴)
static DriveMode driveMode = MODE_MANUAL;

// 루틴의 한 단계(동작, 조향각, 유지시간)
struct TimedAction
{
    BaseAction action;
    float angle;
    unsigned long duration;
};

// 루틴 실행 상태 관리용 구조체
struct RoutineState
{
    bool active;
    int index;
    unsigned long start;
    int length;
    TimedAction* routine;
};

static RoutineState routine =
{
    false,
    0,
    0,
    0,
    nullptr
};

// 기본 동작을 모터 제어 명령으로 변환
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

        case ACT_ROTATE_L:
            setWheelRPM(-30,30);
            break;

        case ACT_ROTATE_R:
            setWheelRPM(30,-30);
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

// 수동 모드에서는 명령을 즉시 실행
void manualDrive(float angle, int action)
{
    executeBaseAction((BaseAction)action, angle);
}

TimedAction logisticsRoutine[] =
{
    {ACT_FORWARD,0,2000},
    {ACT_REVERSE,0,1000},
    {ACT_STOP,0,0}
};

TimedAction finishRoutine[] =
{
    {ACT_ROTATE_R,0,2000},
    {ACT_FORWARD,0,1000},
    {ACT_ROTATE_R,0,2000},
    {ACT_REVERSE,0,2000},
    {ACT_STOP,0,0}
};

// 지정된 루틴을 시작하고 모드를 루틴으로 전환
void startRoutine(int type)
{
    routine.active = true;
    routine.index = 0;
    routine.start = millis();

    driveMode = MODE_ROUTINE;

    switch(type)
    {
        case 1:
            routine.routine = logisticsRoutineIn;
            routine.length = sizeof(logisticsRoutineIn) / sizeof(TimedAction);
            break;

        case 2:
            routine.routine = logisticsRoutineOut;
            routine.length = sizeof(logisticsRoutineOut) / sizeof(TimedAction);
            break;
        case 3:
            routine.routine = finishRoutine;
            routine.length = sizeof(finishRoutine) / sizeof(TimedAction);
            break;

        default:
            routine.active = false;
            driveMode = MODE_MANUAL;
            return;
    }
}

// 시간 기반으로 현재 루틴 단계를 진행
void updateRoutine()
{
    if(!routine.active || routine.routine == nullptr)
        return;

    unsigned long now = millis();
    TimedAction &act = routine.routine[routine.index];

    executeBaseAction(act.action, act.angle);

    if(act.duration == 0)
    {
        routine.active = false;
        driveMode = MODE_MANUAL;
        return;
    }

    if(now - routine.start >= act.duration)
    {
        routine.index++;
        routine.start = now;

        if(routine.index >= routine.length)
        {
            stopMotors();
            routine.active = false;
            driveMode = MODE_MANUAL;
        }
    }
}

// 현재 모드에 따라 수동/루틴 업데이트를 분기
void updateDrive(float angle, int action)
{
    if(driveMode == MODE_MANUAL)
    {
        manualDrive(angle, action);
    }
    else if(driveMode == MODE_ROUTINE)
    {
        updateRoutine();
    }
}
