#include "communication.h"
#include "drive.h"
#include "motor.h"

/*
 * ===== 주요 기능 요약 =====
 * 1) 명령 수신(readCommand)
 * 2) 분류 기반 주행 분기(일반/감속/루틴)
 * 3) 루틴 진행(processRoutine)
 */

int class_;
float angle;
int action;
int pendingMissionClass = 0;

// 차선 주행 중 라이다 값 확인하는 변수
bool flag1 = false, flag2 = false;

// [비기능] 새 명령이 없을 때 사용할 마지막 분류 값 유지
int lastClass = 0;

// [주요 기능] 전체 주행 활성화 상태
bool driveEnabled = false;

void setup()
{
    // [주요 기능] 통신/모터 초기화
    initCommunication();
    initMotor();

    // 드라이브 플래그 True로 설정하고 기본 시작 주행 입력
    driveEnabled = true;
    
}

void loop()
{
    // [주요 기능] 한 줄 명령 파싱이 완료됐을 때만 값 갱신
    if(readCommand(class_, angle, action))
    {
        lastClass = class_;

        // case 1 안에서 사용할 미션 상태를 미리 저장
        if(class_ == 2 || class_ == 5 || class_ == 9)
            pendingMissionClass = class_;
    }

    // [주요 기능] 분류 값 기반 상위 동작 상태 분기
    switch(lastClass)
    {
        case 1: // 일반 주행 + 라이다 단계 처리
            // 비상 정지 (action==9)
            if(action == 9)
            {
                stopMotors();
                driveEnabled = false;
                cancelRoutine();
                flag1 = false;
                flag2 = false;
                break;
            }

            // 1단계: 서행 구간 진입
            if(action == 1)
            {
                flag1 = true;
                if(driveEnabled && !isRoutineActive())
                    processDrive(angle, ACT_SLOW);
                break;
            }
            // 2단계: 제자리 회전 동작
            if(action == 2 && flag1)
            {
                flag2 = true;
                if(driveEnabled && !isRoutineActive())
                    processDrive(angle, ACT_ROTATE_L);
                break;
            }

            // 3단계: 정지 후 미션(class 2/5) 분기
            if ((action == 3) && (flag2 == true)) {
                // 정지 -> 플래그 물류/도착 만들어서 실행하기
                stopMotors();

                if(driveEnabled && !isRoutineActive()){

                    if (pendingMissionClass == 2 || pendingMissionClass == 9){ // 물류면
                        startRoutine(1);
                        if ( pendingMissionClass == 9)
                            startRoutine(2);
                    }
                    else if (pendingMissionClass == 5){ // 도착이면
                        startRoutine(3);
                    }
                }
                flag1 = false;
                flag2 = false;
                break;
            }


            if(driveEnabled && !isRoutineActive())
                processDrive(angle, ACT_FORWARD);
            break;

        // case 1에서 사용할 클래스 ID
        case 2:
        case 5:
        case 9:
            break;

        case 3: // 사람 감지
        case 4: // 자동차 감지 -> 정지
            stopMotors();
            driveEnabled = false;
            cancelRoutine();
            break;

        case 6: // ㅓ자
        case 8: // ㅜ자 -> 좌회전
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, ACT_LEFT);
            break;

        case 7: // 십자가 -> 직진
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, ACT_FORWARD);
            break;

        case 0: // 차선 인식 안됨
            // 이전 조향 유지
            break;


        default:
            // [비기능] 정의되지 않은 분류일 때 동작 필드로 대응
            if(driveEnabled && !isRoutineActive())
                processDrive(angle, action);
            break;
    }

    // [주요 기능] 루틴은 반복 주기마다 독립 진행
    processRoutine();
}
