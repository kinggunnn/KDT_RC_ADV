#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include "config.h"

void initCommunication();
// 쉼표 구분 한 줄 명령을 파싱해 출력 변수에 저장
bool readCommand(int &class_, float &angle, int &action);

#endif
