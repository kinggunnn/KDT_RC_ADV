#ifndef DRIVE_H
#define DRIVE_H

#include "motor.h"

/*
 * ===== 주요 기능 요약 =====
 * 1) processDrive(): 수동 주행 처리
 * 2) startRoutine()/processRoutine(): 루틴 시작 및 진행
 * 3) executeBaseAction(): 기본 동작을 모터 명령으로 변환
 */

// [주요 기능] 수동 주행/루틴 공통 기본 동작
// [비기능] 열거형 값은 외부 동작 값과 매핑되어 사용됨
enum BaseAction
{
    ACT_FORWARD,
    ACT_LEFT,
    ACT_RIGHT,
    ACT_ROTATE_L,
    ACT_ROTATE_R,
    ACT_STOP,
    ACT_SLOW,
    ACT_REVERSE
};

// [주요 기능] 타이머 기반 루틴 실행 중 여부 확인
bool isRoutineActive();

// [주요 기능] 인식 결과(각도/동작) 기반 수동 주행 갱신
void processDrive(float angle, int action);
void executeBaseAction(BaseAction act, float angle);

// [주요 기능] 타이머 기반 루틴 제어
void startRoutine(int routine);
void processRoutine();
void cancelRoutine();

#endif
