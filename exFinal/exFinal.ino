#include "communication.h"
#include "drive.h"
#include "motor.h"

/*
 * action sequence
 * 1 : slow
 * 2 : rotate left 90 deg -> slow forward
 * 3 : stop -> wait camera class(2 or 5) -> run mission
 */

// 최신 수신 원본 데이터
int class_ = 0;
float angle = 0.0f;
int action = 0;

// 마지막으로 정상 파싱된 명령값
int lastClass = 0;
float lastAngle = 0.0f;
int lastAction = 0;

// 전체 주행 허용 상태와 비상정지 래치 상태
bool driveEnabled = false;
bool emergencyLatched = false;
int emergencyClearCount = 0;

// 비상정지 해제와 미션 클래스 확정에 필요한 연속 확인 횟수
const int EMERGENCY_CLEAR_THRESHOLD = 5;
const int MISSION_CONFIRM_THRESHOLD = 3;

enum MissionType
{
    MISSION_NONE,
    MISSION_LOGISTICS,
    MISSION_PARKING
};

// -----------------------------
// 시퀀스 상태
// 0 : idle
// 1 : action1(서행)
// 2 : action2(90도 회전 후 서행 직진)
// 3 : action3(정지 후 카메라 판단 대기)
// -----------------------------
int actionSequenceStep = 0;
bool actionSequenceActive = false;

// action2 내부 상태
bool rotateStarted = false;
bool rotateFinished = false;
unsigned long rotateStartMs = 0;

// 90도 회전 시간 보정값(실차 튜닝 필요)
const unsigned long ROTATE_90_MS = 850;

// 정지 후 카메라 판단용
bool waitingMissionClass = false;
int pendingMissionClass = 0;
// class 2/5를 연속 확인해 미션 시작을 확정하기 위한 상태
int missionCandidateClass = 0;
int missionCandidateCount = 0;
// 현재 수행 중인 미션 종류와 탈출 신호 대기 상태
MissionType activeMission = MISSION_NONE;
bool waitingExitSignal = false;
// 루틴 종료 순간을 감지하기 위한 이전 loop 상태
bool previousRoutineActive = false;
// class 0일 때 유지할 최근 유효 조향각
float heldDriveAngle = 0.0f;

// --------------------------------------------------
// 비상 정지
// --------------------------------------------------
void emergencyStop()
{
    stopMotors();
    driveEnabled = false;
    emergencyLatched = true;
    emergencyClearCount = 0;
    cancelRoutine();

    actionSequenceActive = false;
    actionSequenceStep = 0;
    rotateStarted = false;
    rotateFinished = false;
    waitingMissionClass = false;
    pendingMissionClass = 0;
    missionCandidateClass = 0;
    missionCandidateCount = 0;
    activeMission = MISSION_NONE;
    waitingExitSignal = false;
    previousRoutineActive = false;
}

// --------------------------------------------------
// 시퀀스 초기화
// --------------------------------------------------
void resetActionSequence()
{
    actionSequenceActive = false;
    actionSequenceStep = 0;
    rotateStarted = false;
    rotateFinished = false;
    waitingMissionClass = false;
    pendingMissionClass = 0;
    missionCandidateClass = 0;
    missionCandidateCount = 0;
}

// 사람/차량 인식 시 비상정지를 걸고, 위험 해제 후 일정 횟수 뒤 복귀
void updateEmergencyState(int currentClass)
{
    bool blocked = (currentClass == 3 || currentClass == 4);

    if(blocked)
    {
        if(!emergencyLatched)
            emergencyStop();
        else
            stopMotors();
        return;
    }

    if(!emergencyLatched)
        return;

    emergencyClearCount++;

    if(emergencyClearCount >= EMERGENCY_CLEAR_THRESHOLD)
    {
        emergencyLatched = false;
        emergencyClearCount = 0;
        driveEnabled = true;
        resetActionSequence();
    }
}

// 차선 기반 주행에 유효한 class가 들어오면 최근 조향각을 저장
void updateHeldDriveAngle(int currentClass, float currentAngle)
{
    switch(currentClass)
    {
        case 1:
        case 2:
        case 5:
        case 7:
            heldDriveAngle = currentAngle;
            break;
    }
}

// 미션 루틴이 끝나는 순간을 감지해 class 9 탈출 신호 대기로 전환
void updateMissionRoutineState()
{
    bool routineActiveNow = isRoutineActive();

    if(previousRoutineActive && !routineActiveNow && activeMission != MISSION_NONE)
    {
        waitingExitSignal = true;
    }

    previousRoutineActive = routineActiveNow;
}

// --------------------------------------------------
// 정지 상태에서 class 2/5 판단
// --------------------------------------------------
// 같은 미션 class가 3회 연속 들어와야 물류/주차 시작으로 확정
bool tryCaptureMissionClass(int currentClass)
{
    if(currentClass != 2 && currentClass != 5)
    {
        missionCandidateClass = 0;
        missionCandidateCount = 0;
        return false;
    }

    if(missionCandidateClass != currentClass)
    {
        missionCandidateClass = currentClass;
        missionCandidateCount = 1;
        return false;
    }

    missionCandidateCount++;

    if(missionCandidateCount >= MISSION_CONFIRM_THRESHOLD)
    {
        pendingMissionClass = currentClass;
        missionCandidateClass = 0;
        missionCandidateCount = 0;
        return true;
    }

    return false;
}

// --------------------------------------------------
// action 시퀀스 처리
// true  : 이번 loop에서 action 관련 처리 완료
// false : 일반 class 기반 주행으로 진행
// --------------------------------------------------
// action 1->2->3 순서를 따라 서행, 좌회전, 정지 후 미션 진입을 처리
bool handleActionSequence(int currentClass, float currentAngle, int currentAction)
{
    // -----------------------------
    // 시퀀스 시작 전
    // -----------------------------
    if(!actionSequenceActive)
    {
        if(currentAction == 1)
        {
            actionSequenceActive = true;
            actionSequenceStep = 1;

            rotateStarted = false;
            rotateFinished = false;
            waitingMissionClass = false;
            pendingMissionClass = 0;

            if(driveEnabled && !isRoutineActive())
                processDrive(currentAngle, ACT_SLOW);

            return true;
        }

        return false;
    }

    // =============================
    // STEP 1 : action 1 = 서행
    // =============================
    if(actionSequenceStep == 1)
    {
        if(currentAction == 2)
        {
            actionSequenceStep = 2;
            rotateStarted = false;
            rotateFinished = false;
        }
        else
        {
            if(driveEnabled && !isRoutineActive())
                processDrive(currentAngle, ACT_SLOW);

            return true;
        }
    }

    // =============================
    // STEP 2 : action 2
    // 왼쪽 90도 회전 -> 서행 직진
    // =============================
    if(actionSequenceStep == 2)
    {
        if(!rotateStarted)
        {
            rotateStarted = true;
            rotateFinished = false;
            rotateStartMs = millis();
        }

        // 90도 회전 중
        if(!rotateFinished)
        {
            if(millis() - rotateStartMs < ROTATE_90_MS)
            {
                if(driveEnabled && !isRoutineActive())
                    processDrive(0.0f, ACT_ROTATE_L);

                return true;
            }
            else
            {
                rotateFinished = true;
            }
        }

        // 회전 후에는 서행 직진 유지, action 3 기다림
        if(currentAction == 3)
        {
            actionSequenceStep = 3;
            waitingMissionClass = true;
            stopMotors();
            return true;
        }
        else
        {
            if(driveEnabled && !isRoutineActive())
                processDrive(0.0f, ACT_SLOW);

            return true;
        }
    }

    // =============================
    // STEP 3 : 정지 후 카메라 판단 대기
    // class 2 or 5 들어오면 루틴 실행
    // =============================
    if(actionSequenceStep == 3)
    {
        stopMotors();

        if(waitingMissionClass)
        {
            if(tryCaptureMissionClass(currentClass))
            {
                if(driveEnabled && !isRoutineActive())
                {
                    if(pendingMissionClass == 2)
                    {
                        activeMission = MISSION_LOGISTICS;
                        waitingExitSignal = false;
                        startRoutine(1);
                    }
                    else if(pendingMissionClass == 5)
                    {
                        activeMission = MISSION_PARKING;
                        waitingExitSignal = false;
                        startRoutine(3);
                    }
                }

                resetActionSequence();
                return true;
            }
        }

        return true;
    }

    return true;
}

void setup()
{
    // 통신과 모터를 초기화한 뒤 주행 가능 상태로 시작
    initCommunication();
    initMotor();

    driveEnabled = true;
}

void loop()
{
    // 새 명령이 들어오면 마지막 유효 입력값 갱신
    if(readCommand(class_, angle, action))
    {
        lastClass = class_;
        lastAngle = angle;
        lastAction = action;
    }

    // class 0에서 사용할 이전 조향각 저장
    updateHeldDriveAngle(lastClass, lastAngle);

    // 사람/차량 감지에 따른 비상정지 상태 갱신
    updateEmergencyState(lastClass);

    if(emergencyLatched)
        return;

    // 미션 종료 후에는 class 9가 들어올 때만 탈출 루틴 시작
    if(waitingExitSignal && !isRoutineActive())
    {
        if(lastClass == 9)
        {
            waitingExitSignal = false;
            activeMission = MISSION_NONE;
            startRoutine(2);
        }

        processRoutine();
        updateMissionRoutineState();
        return;
    }

    // 1) action 기반 특수 시퀀스 우선 처리
    if(handleActionSequence(lastClass, lastAngle, lastAction))
    {
        processRoutine();
        updateMissionRoutineState();
        return;
    }

    // 2) 시퀀스가 없을 때만 일반 class 기반 주행 수행
    switch(lastClass)
    {
        case 1:
            if(driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;

        case 2:
        case 5:
            if(driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;

        case 6:
        case 8:
            if(driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_LEFT);
            break;

        case 7:
            if(driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;

        case 0:
            if(driveEnabled && !isRoutineActive())
                processDrive(heldDriveAngle, ACT_FORWARD);
            break;

        case 9:
            break;

        case 3:
        case 4:
            emergencyStop();
            break;

        default:
            if(driveEnabled && !isRoutineActive())
                processDrive(lastAngle, ACT_FORWARD);
            break;
    }

    processRoutine();
    updateMissionRoutineState();
}
