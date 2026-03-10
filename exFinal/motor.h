#ifndef MOTOR_H
#define MOTOR_H

void initMotor();

// 조향각을 좌/우 바퀴 속도로 변환해 적용
void applyAngleDrive(float angleDeg,float speedScale,float bias);
// 좌/우 바퀴 목표 회전수 직접 명령(구현부에서 범위 제한)
void setWheelRPM(float left, float right);
// 즉시 정지
void stopMotors();

#endif
