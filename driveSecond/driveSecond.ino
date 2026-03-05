#include "DriveController.h"

void setup() {
    initializeDriveSystem();
}

void loop() {
    // non-blocking방식
    readCommandSerial(); // 명령 수신 
    updateRoutine();     // 특수 동작 업데이트 
}