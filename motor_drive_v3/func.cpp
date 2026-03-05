#include "func.h"

#include <Dynamixel2Arduino.h>
#include <string.h>
#include <Arduino.h>

// 자동으로 설정
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
  #include <SoftwareSerial.h>
  SoftwareSerial classSoftSerial(7, 8); // 라즈베리파이 명령을 메인 시리얼과 분리해서 받기 위해 사용
  #define DXL_SERIAL Serial        // 다이나믹셀 통신에 사용하는 포트
  #define CLASS_SERIAL classSoftSerial // 라즈베리파이 명령을 읽는 포트
#else
  #define DXL_SERIAL Serial1
  #define CLASS_SERIAL Serial // 별도 설정이 없으면 기본 USB 시리얼로 명령 받기
#endif

// 핀 설정
const int DXL_DIR_PIN = 2; // 다이나믹셀 쉴드의 방향 제어 핀
const uint8_t LEFT_ID = 1; // 왼쪽 바퀴 모터 ID
const uint8_t RIGHT_ID = 2; // 오른쪽 바퀴 모터 ID
const float DXL_PROTOCOL_VERSION = 2.0; // 다이나믹셀 프로토콜 버전

// 다이내믹쉘 생성자 생성
Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN); // 다이나믹셀 모터를 제어하는 객체

// 주행 관련 상수 설정
const float BASE_RPM = 30.0f;
const float MAX_RPM = 60.0f;
const float MIN_RPM = -60.0f;
const float TURN_GAIN_RPM_PER_DEG = 0.6f; // 1도에 좌우 RPM 차이 0.6으로 진행
const float SIDE_BIAS_RPM = 6.0f; // 좌우 치우침 보정용 추가 RPM
const float ANGLE_LIMIT_DEG = 30.0f; // 입력 조향 각도 제한값

// 하드코딩 좌회전 구현
static bool turning_L = false;
static unsigned long turnStart = 0;


/* ===================================================================================
[상태 변수]
driveEnabled:
- 안전을 위해 START(class=9)를 받기 전에는 움직이지 않게 할려고 작성
- STOP(class=10)을 받으면 다시 멈추고 명령 무시

lastAngle:
- 최근 조향각을 저장
- 장애물 회피 동작 후에 원래 조향으로 복귀할 때 사용
=================================================================================== */
bool driveEnabled = false; 
float lastAngle = 0.0f; 

char rxBuf[64]; // 한 줄 명령을 임시로 저장하는 버퍼
uint8_t rxIdx = 0; // 버퍼에 현재 몇 글자가 들어갔는지 저장


// 모터와 시리얼을 초기화하고 처음에는 정지 상태
void initializeDriveSystem() {
  CLASS_SERIAL.begin(115200);

  dxl.begin(1000000);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  // 모터 핑 확인하는 코드 -> 어차피 출력 못함(아두이노랑 DXL 쉴드가 사용하고있어서)
  dxl.ping(LEFT_ID);
  dxl.ping(RIGHT_ID);

  // 왼쪽 오른쪽 토크 끄고
  dxl.torqueOff(LEFT_ID); 
  dxl.torqueOff(RIGHT_ID);

  // 모드 설정하고
  dxl.setOperatingMode(LEFT_ID, OP_VELOCITY);
  dxl.setOperatingMode(RIGHT_ID, OP_VELOCITY);

  // 다시 켜기
  dxl.torqueOn(LEFT_ID);
  dxl.torqueOn(RIGHT_ID);

  stopMotors();
}

// 값이 범위를 넘지 않도록 제한
float clampf(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }

  if (value > maximum) {
    return maximum;
  }

  return value;
}

// 좌우 바퀴 RPM을 제한한 뒤 모터에 전달
void setWheelRPM(float leftRpm, float rightRpm) {
  leftRpm = clampf(leftRpm, MIN_RPM, MAX_RPM);
  rightRpm = clampf(rightRpm, MIN_RPM, MAX_RPM);

  dxl.setGoalVelocity(LEFT_ID, leftRpm, UNIT_RPM);
  dxl.setGoalVelocity(RIGHT_ID, rightRpm, UNIT_RPM);
}

// 두 바퀴를 모두 정지
void stopMotors() {
  setWheelRPM(0.0f, 0.0f);
}

// 조향 각도를 이용해 좌우 바퀴 속도를 계산 : 차동구동 로직
void applyAngleDrive(float angleDeg, float speedScale, float extraBiasRpm) {
  float limitedAngle = clampf(angleDeg, -ANGLE_LIMIT_DEG, ANGLE_LIMIT_DEG);
  // 기본 속도. 스피스 스케일은 감속(횡단보도) 같은 상황에서 곱해서 감속함.
  float baseRpm = BASE_RPM * speedScale;
  // 조향을 위한 차이값을 사용 (TURN_GAIN_RPM_PER_DEG=0.6 [1도])
  float diffRpm = limitedAngle * TURN_GAIN_RPM_PER_DEG + extraBiasRpm;

  float leftRpm = baseRpm + diffRpm;
  float rightRpm = baseRpm - diffRpm;

  setWheelRPM(leftRpm, rightRpm);
}

// 로직 고민
// 주차 명령이 왔을 때 미리 정한 순서대로 움직임 <- 테스트 해보며 작성하기 
void doParkingRoutine() {   
  stopMotors();
  delay(500);

  // 후진 하면서 오른쪽 뒤로 회전
  setWheelRPM(-20.0f, -10.0f);
  delay(2000);

  // 후진만 진행
  setWheelRPM(-20.0f, -20.0f);
  delay(1000);

  // 살짝 앞으로 가서 정렬
  setWheelRPM(15.0f, 15.0f);
  delay(500);

  stopMotors();
}

// 로직 고민
// 차량을 피하기 위한 간단한 회피 동작
void doEvasiveManeuver() {

  // 오른쪽으로 살짝 진행
  setWheelRPM(20.0f, 5.0f);
  delay(500);

  // 반대로 진행
  setWheelRPM(5.0f, 20.0f);
  delay(500);

  // 마지막 각도(lastAngle)로 주행하기
  applyAngleDrive(lastAngle, 1.0f, 0.0f);
}


// -----------------------------------------------------------------------
// 클래스에 따른 동작을 결정 (클래스는 첫번째 정보로 넘어옴 => 1,20.0 이런식)
// -----------------------------------------------------------------------
void classAction(int class_, float angleDeg) {
  //Serial.println(angleDeg);
  lastAngle = angleDeg; // 회피 주행하기 위해서 이전 각도 저장

  // START
  if (class_ == 9) {
    driveEnabled = true;
    applyAngleDrive(lastAngle, 1.0f, 0.0f);
    return;
  }

  // STOP
  if (class_ == 10) {
    driveEnabled = false;
    stopMotors();
    return;
  }

  // START 이전이면 움직임 명령 무시
  if (!driveEnabled) {
    //debugPrintln("[INFO] Drive disabled. Ignoring motion command.");
    return;
  }

  // class에 대한 처리 진행
  switch (class_) {
    // 정상 주행
    case 0:
      applyAngleDrive(angleDeg, 1.0f, 0.0f);
      break;

    // 이전 명령 유지: 아무 것도 하지 않음(현재 속도 유지)
    case 1:
      break;

    // 차선 미검출: 일단 직진
    case 2:
      applyAngleDrive(0.0f, 1.0f, 0.0f);
      break;

    // 주차
    case 3:
      doParkingRoutine();
      break;

    // 사람: 정지
    case 4:
      stopMotors();
      break;

    // 차량: 회피
    case 5:
      doEvasiveManeuver();
      break;

    // 횡단보도: 속도 절반
    case 6:
      applyAngleDrive(angleDeg, 0.5f, 0.0f);
      break;

    // 왼쪽으로 치우침: 오른쪽으로 조금 더 가속(+bias)
    case 7:
      applyAngleDrive(angleDeg, 1.0f, SIDE_BIAS_RPM);
      break;

    // 오른쪽으로 치우침: 왼쪽으로 조금 더 가속(-bias)
    case 8:
      applyAngleDrive(angleDeg, 1.0f, -SIDE_BIAS_RPM);
      break;

    // 정의되지 않은 class
    default:
      break;
  }
}

// 2초동안 -30을 유지하면 90도로 회전.
void turnLeft() {
  unsigned long now = millis();

  // 처음 호출 시 회전 시작
  if (!turning_L) {
    turning_L = true;
    turnStart = now;
  }
  // 1초 동안 직진
  if (now - turnStart < 1000) {
    applyAngleDrive(0.0f, 1.0f, 0.0f);
  }
  // 2초 동안 좌회전
  if ((1000 <= now - turnStart) && (now - turnStart < 3000)) {
    applyAngleDrive(-30.0f, 1.0f, 0.0f);
  }

  // 2초 끝나면 정지
  else {
    stopMotors();
  }
}


// 한 줄 문자열을 class와 angle로 나누기
// 9, 0 이런식으로 보내줘서 콤마가 없으면 class만 온 것으로 angle은 LastAngle 사용
void parseClass(char* line) {
  int class_ = 0;
  float angle = 0.0f;
  char* comma = strchr(line, ','); // line에서 , 찾아서 문자가 존재하는 곳에 포인터 반환, 없으면 null 반환

  if (comma == nullptr) {
    class_ = atoi(line); // 문자를 숫자로
    angle = lastAngle;
  } else {
    *comma = '\0';             // "class,angle"에서 콤마를 문자열 끝처럼 만들어 class 부분 분리
    class_ = atoi(line);          // class 파싱
    angle = atof(comma + 1);   // angle 파싱(콤마 다음부터)
  }

  classAction(class_, angle);
}

// 시리얼에서 문자를 하나씩 읽어 한 줄 명령이 완성되면 실행
void readCommandSerial() {
  while (CLASS_SERIAL.available() > 0) { // 개수가 1개 이상이면
    char c = (char)CLASS_SERIAL.read(); // 여기서 binary를 문자 (0, 12.5)로 변환됨

    if (c == '\r') { // 다음 serial 사이에 \r\n이 있으면 \r은 버리기
      continue;
    }

    if (c == '\n') {
      if (rxIdx > 0) { // 초기화 unsigned int8 0으로 설정
        rxBuf[rxIdx] = '\0'; // 1줄 임시 저장 버퍼에서 C 문자열 끝을 null로 표시
        parseClass(rxBuf);
        rxIdx = 0;
      }
      continue;
    }

    if (rxIdx < sizeof(rxBuf) - 1) {
      rxBuf[rxIdx++] = c;
    } else {
      rxIdx = 0;
    }
  }
}


