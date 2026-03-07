#include <Arduino.h>
#include "drive.h"
#include "motor.h"
/* ====================================================
* 기본 주행 + 시간 + 여러 주행 묶은 루틴(특수 주행)
* ================================================== */

/* ================= Timed Action ================= */
// 시간 고려 행동 구조체
struct TimedAction
{
    BaseAction action; // 어떤 주행 할지
    float angle; // 직진/서행같이 angle 필요할 경우 사용
    int duration; // 몇 ms 동안 유지할지
};

/* ================= 기본 주행 실행 ================= */
void executeBaseAction(BaseAction act, float angle)
{
    switch(act)
    {
        case ACT_FORWARD:
            applyAngleDrive(angle,1.0,0); // angle반영 차동 주행
            break;

        case ACT_LEFT:
            applyAngleDrive(-30,1.0,0); // angle:-30 고정 주행
            break;

        case ACT_RIGHT:
            applyAngleDrive(30,1.0,0);
            break;

        case ACT_ROTATE:
            setWheelRPM(20,-20); // 좌우 바퀴 돌려서 제자리회전
            break;

        case ACT_REVERSE:
            setWheelRPM(-20,-20); // 양쪽 바퀴 모두 뒤로
            break;

        case ACT_STOP:
            stopMotors();
            break;

        case ACT_SLOW:
            applyAngleDrive(angle,0.5,0);
            break;
    }
}

/* ================= 기본 주행 ================= */
static BaseAction currentAction = ACT_STOP;
static unsigned long actionStart = 0;
static int actionDuration = 0;

void processDrive(float angle, int action)
{
    unsigned long now = millis();

    if(action != currentAction) // 새로운 주행이 들어왔는지 확인
    {
        currentAction = action; // 현재 주행 업데이트
        actionStart = now; // 새로운 주행 시간 시작

        switch(action)
        {
            case ACT_LEFT:
                actionDuration = 2000;
                break;

            case ACT_RIGHT:
                actionDuration = 2000;
                break;

            default:
                actionDuration = 0;
        }
    }

    // 현재 주행 시간제한이 있고 주행 시간이 제한보다 넘어가면
    if(actionDuration > 0 && now - actionStart > actionDuration)
    {
        currentAction = ACT_FORWARD; // 직진으로 복귀
        actionDuration = 0;
    }

    executeBaseAction((BaseAction)currentAction, angle); // 현재 주행 실행
}

/* ================= Routine ================= */
static bool routineActive = false; // 특수 주행 진행중 여부
static int routineIndex = 0; // 현재 루틴에서 몇번째 행동 실행 중인지
static unsigned long routineStart = 0; // 현재 단계 시작 시간

// 특수주행 확인
bool isRoutineActive()
{
    return routineActive;
}

/* 물류 주차 루틴 */
TimedAction logisticsRoutine[] =
{
    {ACT_FORWARD,0,2000}, // 직진 2초-> 죄회전함 ㅋㅋㅋㅋ
    {ACT_REVERSE,0,1000}, // 후진 1초 
    {ACT_STOP,0,0} // 정지
};

/* 도착 주차 루틴 */
TimedAction finishRoutine[] =
{
    {ACT_RIGHT,0,20z00}, // 우회전 1.5초 
    {ACT_FORWARD,0,1000}, // 직진 1초
    {ACT_STOP,0,0} // 정지
};

TimedAction* currentRoutine = nullptr; // 현재 실행할 특수 주행 선택

/* ================= Routine 시작 ================= */
void startRoutine(int routine)
{
    routineActive = true;
    routineIndex = 0;
    routineStart = millis();

    actionDuration = 0;

    switch(routine)
    {
        case 1:
            currentRoutine = logisticsRoutine;
            break;

        case 2:
            currentRoutine = finishRoutine;
            break;
    }
}

/* ================= Routine 실행 ================= */
void processRoutine()
{
    if(!routineActive || currentRoutine == nullptr)
        return;

    unsigned long now = millis();
    TimedAction act = currentRoutine[routineIndex];

    executeBaseAction(act.action, act.angle);

    if(act.duration == 0) return;

    if(now - routineStart > act.duration)
    {
        routineIndex++;
        routineStart = now;

        if(currentRoutine[routineIndex].action == ACT_STOP)
        {
            executeBaseAction(ACT_STOP,0);
            routineActive = false;
        }
    }
}