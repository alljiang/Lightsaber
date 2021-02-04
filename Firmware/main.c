
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

#include "Button.h"
#include "Neopixel.h"
#include "Power.h"

#include "pinConfig.h"
#include "timerHandler.h"

//  1000 Hz Button LED Handler
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    millisHandler();
}

void Timer1IntHandler(void) {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    Neopixel_setSolid(0, 100, 0);
}

bool a = false;
void SysTickIntHandler(void) {
    if(a) {
        GPIOPinWrite(ESP_IO0_b, ESP_IO0_p, ESP_IO0_p);
        GPIOPinWrite(ESP_IO2_b, ESP_IO2_p, ESP_IO2_p);
        a = false;
    } else {
        GPIOPinWrite(ESP_IO0_b, ESP_IO0_p, 0);
        GPIOPinWrite(ESP_IO2_b, ESP_IO2_p, 0);
        a = true;
    }
}

void setupPins() {
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_2|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

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
    GPIOPinTypeGPIOOutput(ESP_IO0_b, ESP_IO0_p);
    GPIOPinTypeGPIOOutput(ESP_IO2_b, ESP_IO2_p);
    GPIOPinTypeGPIOOutput(ESP_EN_b, ESP_EN_p);
    GPIOPinTypeGPIOOutput(ESP_RST_b, ESP_RST_p);
    GPIOPinTypeGPIOOutput(CS_SD_b, CS_SD_p);
    GPIOPinTypeGPIOOutput(CS_DAC_b, CS_DAC_p);
}

void setupInterrupts() {
    //  Systick
    SysTickPeriodSet(10);
    SysTickPeriodSet(SysCtlClockGet() / 2);
    SysTickIntEnable();
    SysTickEnable();

    //  Timer 0, Timer 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    IntMasterEnable();

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1000); // 1000 Hz
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 20);  //  20 Hz

    IntEnable(INT_TIMER0A);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER1_BASE, TIMER_A);
}

long last_ButtonHandler = 0;

void controlLoop() {
    if(millis() - last_ButtonHandler >= 10) {
        Button_Handler();
        last_ButtonHandler = millis();
    }
}

int main(void) {
    setupPins();
    setupInterrupts();

    Power_initialize();
    Button_initialize();
    Neopixel_initialize();

    // Latch Power TODO - Delay
    Power_latchOn();

    Button_setSolid(100,100,100,100);
//    Button_setBlink(100,100,100,100, 100, 900);
//    Button_setPulse(0,40,80, 0, 100, 1000, 1000, 800, 700);



    while(1) {
        controlLoop();

        SysCtlDelay(2000000);
    }
}
