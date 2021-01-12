
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "drivers/buttons.h"
#include "tm4c123gh6pm.h"

#include "pinConfig.h"

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

void setup() {
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    // Unlock PF0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE+GPIO_O_CR) |= GPIO_PIN_0;

    // Enable Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}

    // Configure GPIO Pins
    GPIOPinTypeGPIOOutput(PowerON_b, PowerON_p);
    GPIOPinTypeGPIOOutput(Charge_Bias_b, Charge_Bias_p);
    GPIOPinTypeGPIOInput(Not_Charging_b, Not_Charging_p);
    GPIOPinTypeGPIOOutput(ESP_IO0_b, ESP_IO0_p);
    GPIOPinTypeGPIOOutput(ESP_IO2_b, ESP_IO2_p);
    GPIOPinTypeGPIOOutput(ESP_EN_b, ESP_EN_p);
    GPIOPinTypeGPIOOutput(ESP_RST_b, ESP_RST_p);
    GPIOPinTypeGPIOOutput(CS_SD_b, CS_SD_p);
    GPIOPinTypeGPIOOutput(CS_DAC_b, CS_DAC_p);
    GPIOPinTypeGPIOOutput(Neopixel_b, Neopixel_p);

    // Initialize PWM
    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0)) {}

    // Configure PWM Pins
    GPIOPinConfigure(GPIO_PE5_M0PWM5);
    GPIOPinTypePWM(LED_G_b, LED_G_p);

    GPIOPinConfigure(GPIO_PD0_M0PWM6);
    GPIOPinTypePWM(LED_R_b, LED_R_p);

    GPIOPinTypePWM(LED_Anode_b, LED_Anode_p);
    GPIOPinConfigure(GPIO_PB5_M0PWM3);

}

void button_rgb(int redPercent, int greenPercent, int bluePercent, int brightnessPercent) {
    int genPeriod = 10000;

    int redPeriod = (genPeriod / 100 / 100) * redPercent * brightnessPercent / 3;
    int bluePeriod = (genPeriod / 100 / 100) * bluePercent * brightnessPercent / 3 + redPeriod;
    int greenPeriod = genPeriod - ((genPeriod / 100 / 100) * greenPercent * brightnessPercent / 3);

    if(redPeriod <= 0) redPeriod = 1;
    if(redPeriod >= genPeriod) redPeriod = genPeriod - 1;
    if(greenPeriod <= 0) greenPeriod = 1;
    if(greenPeriod >= genPeriod) greenPeriod = genPeriod - 1;
    if(bluePeriod <= 0) bluePeriod = 1;
    if(bluePeriod >= genPeriod) bluePeriod = genPeriod - 1;

    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, genPeriod);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, genPeriod);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, genPeriod);


    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, redPeriod);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, greenPeriod);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, bluePeriod);

//    PWM0_INVERT_R |= 0b1101000;
    PWM0_INVERT_R |= 0b1001000;
    PWMOutputInvert(PWM0_BASE, PWM_OUT_6 | PWM_OUT_5, true);
    PWMOutputInvert(PWM0_BASE, PWM_OUT_3, false);

    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT | PWM_OUT_5_BIT | PWM_OUT_6_BIT, true);

}

int main(void) {
    setup();

    // Latch Power TODO - Delay
    GPIOPinWrite(PowerON_b, PowerON_p, PowerON_p);

    button_rgb(100, 100, 100, 30);

    while(1) {
        SysCtlDelay(2000000);
    }
}
