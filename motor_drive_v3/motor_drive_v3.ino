#include "func.h"

// 라즈베리파이한테 클래스 번호와 조향을 받음
// -> 0, -12.5 = 정상주행, 왼쪽으로 12.5도 꺽어라

// 전원이 켜진 뒤 한 번만 실행되며 전체 주행 환경을 준비한다.
void setup() {
  initializeDriveSystem();
}

// 계속 반복되며 라즈베리파이에서 온 명령을 읽는다.
void loop() {
  //readCommandSerial();
  turnLeft();
}
