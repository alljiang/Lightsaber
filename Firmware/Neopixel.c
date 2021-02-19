
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "driverlib/rom_map.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"

#include "pinConfig.h"
#include "Neopixel.h"

// LED Colors, 24-bit color: GGGGGGGG RRRRRRRR BBBBBBBB
uint32_t data[NEOPIXEL_COUNT];

void Neopixel_initialize() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1)) {}

    GPIOPinConfigure(GPIO_PF1_SSI1TX);

    GPIOPinTypeSSI(Neopixel_b, Neopixel_p);
    SSIIntClear(SSI1_BASE,SSI_TXEOT);
    SSIConfigSetExpClk(SSI1_BASE, 80000000, SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER, 6400000, 8);
    SSIEnable(SSI1_BASE);
}

void Neopixel_sendData(uint32_t values[],int number) {
    GPIOPinConfigure(GPIO_PF1_SSI1TX);
    GPIOPinTypeSSI(Neopixel_b, Neopixel_p);
    int k, i;
    for(k = 0; k < number; k++) {
        for(i = 23; i >= 0; i--) {
            volatile uint8_t convert = 0xC0 ;//0b11000000;
            if((values[k] >> i) & 0x1 != 0) convert = 0xF8;// 0b11111000;
            SSIDataPut(SSI1_BASE, convert);
        }
    }
    GPIOPinTypeGPIOOutput(Neopixel_b, Neopixel_p);
    GPIOPinWrite(Neopixel_b, Neopixel_p, 0);
}

void Neopixel_setSolid(uint8_t r, uint8_t g, uint8_t b) {


//    uint32_t grb = 0b000000001111111111111111;
//    uint32_t grb = 0b000000001111111100000000;
    uint32_t grb = 0b000000000000000011111111;
    int i;
    for(i = 0; i < 140; i++) {
        data[i] = grb;
    }

    Neopixel_sendData(data, 140);
}
