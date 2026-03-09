#include "communication.h"

#include <Arduino.h>
#include <string.h>

/*
 * ===== 주요 기능 요약 =====
 * 1) initCommunication(): 명령 수신 시리얼 초기화
 * 2) readCommand(): 한 줄 명령을 파싱해 분류/각도/동작으로 변환
 */

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
#include <SoftwareSerial.h>
SoftwareSerial classSoftSerial(7,8);
#define CLASS_SERIAL classSoftSerial
#else
#define CLASS_SERIAL Serial
#endif

// [비기능] 수신 명령을 임시 저장하는 버퍼 상태(파싱 보조용)
char rxBuf[64];
uint8_t rxIdx = 0;

// [주요 기능] 명령 수신 시리얼 채널 초기화
void initCommunication()
{
    // [비기능] 통신 속도 적용
    CLASS_SERIAL.begin(COMM_BAUDRATE);
}

// [주요 기능] 시리얼 한 줄 명령을 읽어 분류/각도/동작 값으로 저장
bool readCommand(int &class_, float &angle, int &action)
{
    while(CLASS_SERIAL.available() > 0)
    {
        char c = CLASS_SERIAL.read();

        // [비기능] 제어 문자 무시(파싱 안정성)
        if(c == '\r') continue;

        if(c == '\n')
        {
            if(rxIdx == 0) return false; // [비기능] 빈 줄 무시

            // [비기능] 문자열 종료 처리
            rxBuf[rxIdx] = '\0';

            // [주요 기능] 쉼표 기준 필드 분리
            char *p1 = strtok(rxBuf,",");
            char *p2 = strtok(NULL,",");
            char *p3 = strtok(NULL,",");

            if(p1 == NULL) return false;

            class_ = atoi(p1);
            angle = (p2 != NULL) ? atof(p2) : 0;
            action = (p3 != NULL) ? atoi(p3) : 0;

            rxIdx = 0;
            return true;
        }

        // [주요 기능] 버퍼 한도 내 수신 데이터 누적
        if(rxIdx < sizeof(rxBuf)-1)
            rxBuf[rxIdx++] = c;
        else
            rxIdx = 0; // [비기능] 오버플로 방지
    }

    return false;
}
