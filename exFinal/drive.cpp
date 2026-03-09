#include <Arduino.h>
#include "drive.h"
#include "motor.h"

/*
 * ===== 주요 기능 요약 =====
 * 1) processDrive(): 수동 주행 명령 처리
 * 2) startRoutine()/processRoutine(): 타이머 기반 루틴 주행
 * 3) executeBaseAction(): 기본 동작을 모터 명령으로 변환
 */

// [비기능] 루틴의 단일 동작 단계 정의
struct TimedAction
{
    BaseAction action;
    float angle;
    unsigned long duration;
};

/* ================= [비기능] 루틴 상태 ================= */
static bool routineActive = false;
static int routineIndex = 0;
static unsigned long routineStart = 0;
static int routineLength = 0;
static TimedAction* currentRoutine = nullptr;

/* ================= [비기능] 수동 주행 상태 ================= */
// 좌/우회전 같은 임시 동작을 일정 시간 후 복귀시키기 위한 상태
static BaseAction currentAction = ACT_STOP;
static unsigned long actionStart = 0;
static unsigned long actionDuration = 0;

// [주요 기능] 기본 동작을 실제 모터 제어 함수로 매핑
void executeBaseAction(BaseAction act, float angle)
{
    switch(act)
    {
        case ACT_FORWARD:
            applyAngleDrive(angle, 1.0, 0);
            break;

        case ACT_LEFT:
            applyAngleDrive(-30, 1.0, 0);
            break;

        case ACT_RIGHT:
            applyAngleDrive(30, 1.0, 0);
            break;

        case ACT_ROTATE_L:
            setWheelRPM(-30, 30);
            break;

        case ACT_ROTATE_R:
            setWheelRPM(30, -30);
            break;

        case ACT_REVERSE:
            setWheelRPM(-20, -20);
            break;

        case ACT_STOP:
            stopMotors();
            break;

        case ACT_SLOW:
            applyAngleDrive(angle, 0.5, 0);
            break;
    }
}

// [주요 기능] 수동 주행 명령 처리
void processDrive(float angle, int action)
{
    // [비기능] 루틴 동작 중에는 수동 명령 무시
    if(routineActive) return;

    unsigned long now = millis();

    // [주요 기능] 명령 변경 시 타이머 상태 갱신
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

    // [주요 기능] 좌/우회전 제한 시간 종료 시 전진 복귀
    if(actionDuration > 0 && now - actionStart > actionDuration)
    {
        currentAction = ACT_FORWARD;
        actionDuration = 0;
    }

    executeBaseAction(currentAction, angle);
}

// [주요 기능] 물류 루틴 시퀀스
TimedAction logisticsRoutineIn[] =
{
    {ACT_FORWARD,0,2000},
    {ACT_REVERSE,0,1000},
    {ACT_STOP,0,0}
};

TimedAction logisticsRoutineOut[] =
{
    {ACT_FORWARD,0,2000},
    {ACT_REVERSE,0,1000},
    {ACT_STOP,0,0}
};

// [주요 기능] 공사 구간 루틴 시퀀스
TimedAction finishRoutine[] =
{
    {ACT_ROTATE_R,0,2000},
    {ACT_FORWARD,0,1000},
    {ACT_ROTATE_R,0,2000},
    {ACT_REVERSE,0,2000},
    {ACT_STOP,0,0}
};

// [주요 기능] 루틴 실행 중 여부 반환
bool isRoutineActive()
{
    return routineActive;
}

// [주요 기능] 루틴 강제 취소
void cancelRoutine()
{
    routineActive = false;
}

// [주요 기능] 지정 루틴 시작
void startRoutine(int routine)
{
    routineActive = true;
    routineIndex = 0;
    routineStart = millis();

    // [비기능] 루틴 시작 시 수동 동작 타이머 초기화
    actionDuration = 0;
    currentAction = ACT_STOP;

    switch(routine)
    {
        case 1:
            currentRoutine = logisticsRoutineIn;
            routineLength = sizeof(logisticsRoutineIn) / sizeof(TimedAction);
            break;

        case 2:
            currentRoutine = logisticsRoutineOut;
            routineLength = sizeof(logisticsRoutineOut) / sizeof(TimedAction);
            break;
        case 3:
            currentRoutine = finishRoutine;
            routineLength = sizeof(finishRoutine) / sizeof(TimedAction);
            break;

        default:
            // [비기능] 잘못된 루틴 번호 방어 처리
            routineActive = false;
            currentRoutine = nullptr;
            routineLength = 0;
            break;
    }
}

// [주요 기능] 루틴 상태를 시간 기준으로 한 단계씩 진행
void processRoutine()
{
    if(!routineActive || currentRoutine == nullptr)
        return;

    unsigned long now = millis();
    TimedAction &act = currentRoutine[routineIndex];

    executeBaseAction(act.action, act.angle);

    // [주요 기능] 유지 시간이 0이면 루틴 종료
    if(act.duration == 0)
    {
        routineActive = false;
        return;
    }

    if(now - routineStart >= act.duration)
    {
        routineIndex++;
        routineStart = now;

        // [비기능] 배열 범위 보호 및 안전 정지
        if(routineIndex >= routineLength)
        {
            executeBaseAction(ACT_STOP,0);
            routineActive = false;
            return;
        }
    }
}
