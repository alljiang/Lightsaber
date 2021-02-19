
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

uint32_t allWhite[NEOPIXEL_COUNT];
uint32_t allOff[NEOPIXEL_COUNT];
bool powerOn;
bool lastButtonState;
bool neopixelState;

//  1000 Hz Button LED Handler
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    millisHandler();
}

void Timer1IntHandler(void) {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
}

void SysTickIntHandler(void) {
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

long last_ButtonLEDHandler = 0;
long last_ButtonPressHandler = 0;
long last_NeopixelHandler = 0;
long last_VoltageHandler = 0;

void controlLoop() {
    long time = millis();

    if(time - last_ButtonLEDHandler >= 10) {
        last_ButtonLEDHandler = time;
        Button_Handler();
    }

    if(time - last_ButtonPressHandler >= 300) {
        last_ButtonPressHandler = time;

        bool buttonState = Button_isPressed();
        if(buttonState && !lastButtonState) {
            powerOn = !powerOn;
        }
        lastButtonState = buttonState;
    }

//    if(time - last_VoltageHandler >= 1000) {
//        last_VoltageHandler = time;
//
//        int x = Power_getBatteryVoltage();
//        if(x < 100) {
//            Power_latchOff();
//        }
//    }

    if(time - last_NeopixelHandler >= 50) {
        last_NeopixelHandler = time;

        if(powerOn) {
            if(neopixelState == 0) {
                neopixelState = 1;
                Neopixel_sendData(allWhite, NEOPIXEL_COUNT);
            } else if(neopixelState == 1) {
                neopixelState = 0;
                Neopixel_sendData(allWhite, NEOPIXEL_COUNT);

            }
        } else {
            neopixelState = 0;
            Neopixel_sendData(allOff, NEOPIXEL_COUNT);
        }
    }
}

int main(void) {
    int i;

    setupPins();

    Power_initialize();
    Button_initialize();
    Neopixel_initialize();

    setupInterrupts();

    // Latch Power TODO - Delay
    Power_latchOn();

//    Button_setSolid(100,100,100,100);
//    Button_setBlink(100,100,100,100, 100, 900);
//    Button_setPulse(0,40,80, 0, 100, 1000, 1000, 800, 700);
    Button_setPulse(100,100,100, 0, 50, 1000, 1000, 800, 700);

    lastButtonState = false;
    neopixelState = false;
    for(i = 0 ; i < NEOPIXEL_COUNT; i++) {
        allWhite[i] = 0b111111111111111111111111;
        allOff[i]   = 0b000000000000000000000000;
    }

    while(1) {
        controlLoop();

//        SysCtlDelay(2000000);
    }
}
