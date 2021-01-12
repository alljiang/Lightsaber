
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
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "drivers/buttons.h"
#include "driverlib/timer.h"

#include "pinConfig.h"
#include "buttonLED.h"
#include "timerHandler.h"

#ifdef DEBUG void __error__(char *pcFilename, uint32_t ui32Line) {}
#endif

//  1000 Hz Button LED Handler
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    millisHandler();
}

void Timer1IntHandler(void) {
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
}

void setupPins() {
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

void setupInterrupts() {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    IntMasterEnable();

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1000); // Button LED, 1000 Hz
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet());

    IntEnable(INT_TIMER0A);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER1_BASE, TIMER_A);
}

long lastButtonLEDHandler = 0;
void controlLoop() {
    if(millis() - lastButtonLEDHandler >= 10) {
        buttonLEDHandler();
        lastButtonLEDHandler = millis();
    }

}

int main(void) {
    setupPins();
    setupInterrupts();

    // Latch Power TODO - Delay
    GPIOPinWrite(PowerON_b, PowerON_p, PowerON_p);

//    ButtonLED_setBlink(100,0,100,100, 100, 900);
    ButtonLED_setPulse(100,0,100, 0, 100, 1000, 1000, 800, 700);

    while(1) {
        controlLoop();
    }
}
