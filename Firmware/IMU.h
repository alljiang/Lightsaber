
#ifndef IMU_H_
#define IMU_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define SELF_TEST_X_GYRO 0x00
#define SELF_TEST_Y_GYRO 0x01
#define SELF_TEST_Z_GYRO 0x02
#define SELF_TEST_X_ACCEL 0x0D
#define SELF_TEST_Y_ACCEL 0x0E
#define SELF_TEST_Z_ACCEL 0x0F

#define SMPLRT_DIV 0x19
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_CONFIG_2 0x1D

#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H 0x41
#define TEMP_OUT_L 0x42
#define GYRO_XOUT_H 0x43
#define GYRO_XOUT_L 0x44
#define GYRO_YOUT_H 0x45
#define GYRO_YOUT_L 0x46
#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_L 0x48

#define USER_CTRL 0x6A
#define PWR_MGMT_1 0x6B

#define WHO_AM_I 0x75

extern void IMU_initialize();

extern void IMU_update();

extern int32_t IMU_getTemperature();    //  Celsius, scaled by 100
extern int64_t IMU_getYaw();    //  dps, scaled by 100
extern int64_t IMU_getPitch();  //  dps, scaled by 100
extern int64_t IMU_getRoll();   //  dps, scaled by 100

extern uint32_t IMU_getAngularVelocity();

#endif /* IMU_H_ */
