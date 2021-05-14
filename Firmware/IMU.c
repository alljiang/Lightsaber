
#include "IMU.h"
#include "driverlib/I2C.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "Utils.h"
#include "pinConfig.h"

/*
 *  MPU6500 I2C Address: 1101000
 */

#define gyro_filter_coef 0/100
#define gyro_filter_coef_comp 100/100

int64_t yaw, pitch, roll; //  dps, scaled by 100
int32_t temp = 0;  // scaled by 100

int g_x=0, g_y=0, g_z=0;
int accel_x_hpf, accel_y_hpf, accel_z_hpf;

void I2C_write8(uint8_t reg, uint8_t data) {
    I2CMasterSlaveAddrSet(I2C1_BASE, 0b1101000, false);

    I2CMasterDataPut(I2C1_BASE, reg);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    I2CMasterDataPut(I2C1_BASE, data);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    while(I2CMasterBusy(I2C1_BASE));
}

int8_t I2C_receive8(uint8_t reg) {
    I2CMasterSlaveAddrSet(I2C1_BASE, 0b1101000, false);
    I2CMasterDataPut(I2C1_BASE, reg);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C1_BASE));

    I2CMasterSlaveAddrSet(I2C1_BASE, 0b1101000, true);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
    while(I2CMasterBusy(I2C1_BASE));
    return I2CMasterDataGet(I2C1_BASE);

}

int16_t I2C_receive16(uint8_t high_addr, uint8_t low_addr) {
    uint8_t low = I2C_receive8(low_addr);
    uint8_t high = I2C_receive8(high_addr);

    return (high << 8) | low;
}

void IMU_initialize() {
    // initialize I2C
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);

    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    GPIOPinTypeI2CSCL(SCL_b, SCL_p);
    GPIOPinTypeI2C(SDA_b, SDA_p);

    I2CMasterInitExpClk(I2C1_BASE, SysCtlClockGet(), true);

    I2C_write8(GYRO_CONFIG, 0b00011000);    //  full scale 2000dps
}

void IMU_update() {
    int64_t roll_raw = I2C_receive16(GYRO_XOUT_H, GYRO_XOUT_L);
    int64_t pitch_raw = I2C_receive16(GYRO_YOUT_H, GYRO_YOUT_L);
    int64_t yaw_raw = I2C_receive16(GYRO_ZOUT_H, GYRO_ZOUT_L);
    int64_t temp_raw = I2C_receive16(TEMP_OUT_H, TEMP_OUT_L);

    int64_t roll_converted = roll_raw * 1000 * 100 / 65535;
    int64_t pitch_converted = pitch_raw * 1000 * 100 / 65535;
    int64_t yaw_converted = yaw_raw * 1000 * 100 / 65535;

    //  add complementary low-pass filter to gyro to remove noise
    yaw = yaw * gyro_filter_coef + yaw_converted * gyro_filter_coef_comp;
    pitch = pitch * gyro_filter_coef + pitch_converted * gyro_filter_coef_comp;
    roll = roll * gyro_filter_coef + roll_converted * gyro_filter_coef_comp;

    temp = 10000*(temp_raw - 0)/33387 + 2100;

}

// celsius, scaled by 100
int32_t IMU_getTemperature() {
    return temp;
}

//  dps, scaled by 100
int64_t IMU_getYaw() {
    return yaw;
}

//  dps, scaled by 100
int64_t IMU_getPitch() {
    return pitch;
}

//  dps, scaled by 100
int64_t IMU_getRoll() {
    return roll;
}

//  dps, scaled by 100
#define BOX_FILTER_SIZE 6
uint32_t boxFilterHistory[BOX_FILTER_SIZE];
uint8_t boxFilterPointer = 0;
int64_t boxFilterSum = 0;
uint32_t IMU_getAngularVelocity() {
    uint32_t currentVelocity = sqrtInt(yaw*yaw + pitch*pitch + roll*roll);

    boxFilterSum -= boxFilterHistory[boxFilterPointer];
    boxFilterSum += currentVelocity;
    boxFilterHistory[boxFilterPointer] = currentVelocity;
    boxFilterPointer = (boxFilterPointer+1) % BOX_FILTER_SIZE;

//    int64_t boxFilterSum = 0;
//    for(int i = 0; i < BOX_FILTER_SIZE; i++) boxFilterSum += boxFilterHistory[i];

    return boxFilterSum / BOX_FILTER_SIZE;
}
