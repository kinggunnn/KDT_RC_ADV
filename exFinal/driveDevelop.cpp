#include <Arduino.h>
#include "drive.h"
#include "motor.h"

/* ================= Drive Mode ================= */
// 현재 주행시스템이 무엇인지 나타냄.
enum DriveMode
{
    MODE_MANUAL, //일반주행(angle, action에 따라 바로 주행)
    MODE_ROUTINE //루틴주행(미리 정해둔 루틴 순서대로 수행)
};

static DriveMode driveMode = MODE_MANUAL; //현재 어떤 주행 중인지

/* ================= Timed Action ================= */
// 주행 관련 구조체.
struct TimedAction
{
    BaseAction action; // 어떤 주행할지(직진, 좌회전, ...)
    float angle; // 조향이 필요한 동작일때 사용할 값
    unsigned long duration; // 해당 동작을 몇 ms동안 유지할지
};

/* ================= Routine State ================= */
// 현재 실행 중인 루틴의 상태 저장
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
    false,  //비활성
    0,      //인덱스
    0,      //시작시간 0(초기)
    0,      //길이 0(초기)
    nullptr //현재 루틴 없음
};

/* ================= 기본 주행 실행 ================= */
//실제 주행 명령을 모터 제어로
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

/* ================= 일반 주행 ================= */
// 일반 주행 처리
void manualDrive(float angle, int action)
{
    executeBaseAction((BaseAction)action, angle);
}

/* ================= Routine 정의 ================= */
//action, angle, duration
TimedAction logisticsRoutine[] =
{
    {ACT_FORWARD,0,2000},
    {ACT_REVERSE,0,1000},
    {ACT_STOP,0,0}
};

TimedAction finishRoutine[] =
{   
    {ACT_ROTATE,0,2000},
    {ACT_FORWARD,0,1000},
    {ACT_ROTATE,0,2000},
    {ACT_REVERSE,0,2000},
    {ACT_STOP,0,0}
};

/* ================= Routine 시작 ================= */
// 루틴 시작 준비
void startRoutine(int type)
{
    routine.active = true;      //루틴 시작 준비
    routine.index = 0;          //루틴 첫번째 단계부터 시작
    routine.start = millis();   //현재 단계 시작 시간 기록

    driveMode = MODE_ROUTINE;   //루틴 모드로

    switch(type)
    {
        case 1:
            routine.routine = logisticsRoutine;
            routine.length = sizeof(logisticsRoutine) / sizeof(TimedAction);
            break;

        case 2:
            routine.routine = finishRoutine;
            routine.length = sizeof(finishRoutine) / sizeof(TimedAction);
            break;

        default:
            routine.active = false;
            driveMode = MODE_MANUAL;
            return;
    }
}

/* ================= Routine 실행 ================= */
void updateRoutine()
{
    //루틴이 비활성상태거나 루틴 포인터x -> 동작x
    if(!routine.active || routine.routine == nullptr)
        return;

    unsigned long now = millis(); //현재 시간 읽고

    TimedAction &act = routine.routine[routine.index]; // 단계 가져온다

    executeBaseAction(act.action, act.angle); //현재 단계 실행

    if(act.duration == 0)
    {
        routine.active = false;
        driveMode = MODE_MANUAL;
        return;
    }

    //시간만료 검사
    if(now - routine.start >= act.duration)
    {
        routine.index++;
        routine.start = now;
        //배열 끝 검사(범위 보호)
        if(routine.index >= routine.length)
        {
            stopMotors();
            routine.active = false;
            driveMode = MODE_MANUAL;
        }
    }
}

/* ================= Drive Update ================= */
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