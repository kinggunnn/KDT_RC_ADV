#include "func.h"

// ---  객체 및 변수 실제 정의  ---
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
  #include <SoftwareSerial.h>
  SoftwareSerial classSoftSerial(7, 8); 
  #define CLASS_SERIAL classSoftSerial
#else
  #define CLASS_SERIAL Serial
#endif

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
bool driveEnabled = false;
float lastAngle = 0.0f;
static bool ledState = false; // 수신 확인용 LED 제어 변수

// 이 파일에서만 사용하는 내부 변수
static char rxBuf[64];
static uint8_t rxIdx = 0;

/* ===================================================================================
[비차단 루틴 제어 시스템]
currentRoutine: 현재 실행 중인 특수 동작 상태
stateStartTime: 해당 상태가 시작된 시간 (millis)
=================================================================================== */
enum RoutineState { 
    STATE_IDLE, 
    STATE_PARK_1, STATE_PARK_2, STATE_PARK_3, 
    STATE_EVASIVE_1, STATE_EVASIVE_2 
};
static RoutineState currentRoutine = STATE_IDLE;
static uint32_t stateStartTime = 0;

// --- 하드웨어 제어 ---

void initializeDriveSystem() {
    // LED 핀 초기화
    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(DEBUG_LED, LOW);

    CLASS_SERIAL.begin(RPI_SERIAL_BAUD);
    dxl.begin(DXL_SERIAL_BAUD);
    dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

    // 핑 확인 후 토크 및 모드 설정
    if(dxl.ping(LEFT_ID) && dxl.ping(RIGHT_ID)) {
        // setOperatingMode위해 토크가 꺼져 있어야함.
        dxl.torqueOff(LEFT_ID);
        dxl.torqueOff(RIGHT_ID);
        // 속도 제어. 정지 명령 있을때까지 그 속도로 계속 회전.
        dxl.setOperatingMode(LEFT_ID, OP_VELOCITY);
        dxl.setOperatingMode(RIGHT_ID, OP_VELOCITY);
        // 설정 후, 제어 활성화
        dxl.torqueOn(LEFT_ID);
        dxl.torqueOn(RIGHT_ID);
    }
    stopMotors();
}

float clampf(float value, float minimum, float maximum) {
    return (value < minimum) ? minimum : (value > maximum ? maximum : value);
}

void setWheelRPM(float leftRpm, float rightRpm) {
    leftRpm = clampf(leftRpm, MIN_RPM, MAX_RPM);
    rightRpm = clampf(rightRpm, MIN_RPM, MAX_RPM);
    dxl.setGoalVelocity(LEFT_ID, leftRpm, UNIT_RPM);
    dxl.setGoalVelocity(RIGHT_ID, rightRpm, UNIT_RPM);
}

void stopMotors() { 
    setWheelRPM(0.0f, 0.0f); 
}

// --- 주행 시나리오 및 알고리즘 ---

void applyAngleDrive(float angleDeg, float speedScale, float extraBiasRpm) {
    // 특수 루틴 수행 중에는 일반 주행 명령을 무시함 (안전)
    if (!driveEnabled || currentRoutine != STATE_IDLE) return;
    
    float limitedAngle = clampf(angleDeg, -ANGLE_LIMIT_DEG, ANGLE_LIMIT_DEG);
    float baseRpm = BASE_RPM * speedScale;
    
    // 차동 조향 공식: RPM 차이를 이용한 회전
    float diffRpm = (limitedAngle * TURN_GAIN) + extraBiasRpm;
    setWheelRPM(baseRpm + diffRpm, baseRpm - diffRpm);
}

/* ===================================================================================
updateRoutine: loop()에서 무한 반복 호출되며 특수 동작을 단계별로 실행
=================================================================================== */
void updateRoutine() {
    if (currentRoutine == STATE_IDLE) return;

    uint32_t elapsed = millis() - stateStartTime;

    switch (currentRoutine) {
        // [주차 루틴]
        case STATE_PARK_1: // 후진 회전 (2초)
            setWheelRPM(-20.0f, -10.0f);
            if (elapsed >= 2000) { 
                currentRoutine = STATE_PARK_2; 
                stateStartTime = millis(); 
            }
            break;
        case STATE_PARK_2: // 후진 직진 (1초)
            setWheelRPM(-20.0f, -20.0f);
            if (elapsed >= 1000) { 
                currentRoutine = STATE_PARK_3; 
                stateStartTime = millis(); 
            }
            break;
        case STATE_PARK_3: // 정렬 전진 (0.5초)
            setWheelRPM(15.0f, 15.0f);
            if (elapsed >= 500) { 
                stopMotors(); 
                currentRoutine = STATE_IDLE; 
            }
            break;

        // [회피 루틴]
        case STATE_EVASIVE_1: // 오른쪽 회피 (0.5초)
            setWheelRPM(20.0f, 5.0f);
            if (elapsed >= 500) { 
                currentRoutine = STATE_EVASIVE_2; 
                stateStartTime = millis(); 
            }
            break;
        case STATE_EVASIVE_2: // 왼쪽으로 복귀 (0.5초)
            setWheelRPM(5.0f, 20.0f);
            if (elapsed >= 500) { 
                currentRoutine = STATE_IDLE; 
            }
            break;

        default: break;
    }
}

// 기존 delay 방식 함수는 사용하지 않으므로 비워두거나 제거 가능
void doParkingRoutine() {
    currentRoutine = STATE_PARK_1;
    stateStartTime = millis();
}

void doEvasiveManeuver() {
    currentRoutine = STATE_EVASIVE_1;
    stateStartTime = millis();
}

// --- 명령 수신 및 의사결정 ---

void classAction(int classCode, float angleDeg) {
    lastAngle = angleDeg;

    // START/STOP은 무조건 최우선 처리
    if (classCode == CLASS_START) { 
        driveEnabled = true; 
        currentRoutine = STATE_IDLE; // 루틴 초기화
        applyAngleDrive(lastAngle, 1.0f, 0.0f); 
        return; 
    }
    if (classCode == CLASS_STOP) { 
        driveEnabled = false; 
        currentRoutine = STATE_IDLE; // 진행 중인 모든 루틴 즉시 취소
        stopMotors(); 
        return; 
    }
    
    // 주행 비활성화 상태면 아래 로직 수행 안 함
    if (!driveEnabled) return;

    // 특수 루틴이 작동 중일 때는 센서 명령 기반의 조향을 일시 중단 (루틴 우선)
    switch (classCode) {
        case CLASS_NORMAL:    
            applyAngleDrive(angleDeg, 1.0f, 0.0f); 
            break;
        case CLASS_KEEP:      
            break;
        case CLASS_NO_LANE:   
            applyAngleDrive(0.0f, 0.8f, 0.0f);     
            break;
        case CLASS_PARKING:   
            doParkingRoutine();                   
            break;
        case CLASS_HUMAN:     
            currentRoutine = STATE_IDLE; 
            stopMotors(); break; // 사람 발견 시 루틴 중단 및 즉시 정지
        case CLASS_OBSTACLE:  
            doEvasiveManeuver();                  
            break;
        case CLASS_CROSSWALK: 
            applyAngleDrive(angleDeg, 0.5f, 0.0f); 
            break;
        case CLASS_BIAS_LEFT: 
            applyAngleDrive(angleDeg, 1.0f, SIDE_BIAS_RPM); 
            break;
        case CLASS_BIAS_RIGHT:
            applyAngleDrive(angleDeg, 1.0f, -SIDE_BIAS_RPM); 
            break;
        default: 
            break; 
    }
}

void parseCommand(char* line) {
    char* token = strtok(line, ",");
    if (token == NULL) return;
    
    int classCode = atoi(token);
    float angle = lastAngle; 
    
    token = strtok(NULL, ",");
    if (token != NULL) {
        angle = atof(token);
    }
    
    classAction(classCode, angle);
}

void readCommandSerial() {
    while (CLASS_SERIAL.available() > 0) {
        char c = CLASS_SERIAL.read();
        
        if (c == '\n') {
            rxBuf[rxIdx] = '\0';
            if (rxIdx > 0) {
                parseCommand(rxBuf);
                ledState = !ledState;
                digitalWrite(DEBUG_LED, ledState ? HIGH : LOW);
            }
            rxIdx = 0;
        } else if (c != '\r' && rxIdx < sizeof(rxBuf) - 1) {
            rxBuf[rxIdx++] = c;
        }
    }
}