
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
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
#include "tm4c123gh6pm.h"

#include "pinConfig.h"
#include "buttonLED.h"
#include "timerHandler.h"

//  1000 Hz Button LED Handler
void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    millisHandler();
}

void Timer1IntHandler(void) {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
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
    GPIOPinTypeGPIOOutput(PowerON_b, PowerON_p);
    GPIOPinTypeGPIOOutput(Charge_Bias_b, Charge_Bias_p);
    GPIOPinTypeGPIOInput(Not_Charging_b, Not_Charging_p);
    GPIOPinTypeGPIOOutput(ESP_IO0_b, ESP_IO0_p);
    GPIOPinTypeGPIOOutput(ESP_IO2_b, ESP_IO2_p);
    GPIOPinTypeGPIOOutput(ESP_EN_b, ESP_EN_p);
    GPIOPinTypeGPIOOutput(ESP_RST_b, ESP_RST_p);
    GPIOPinTypeGPIOOutput(CS_SD_b, CS_SD_p);
    GPIOPinTypeGPIOOutput(CS_DAC_b, CS_DAC_p);

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

uint32_t data[144];

void Neopixel_sendData(uint32_t values[],int number) {
    GPIOPinConfigure(GPIO_PF1_SSI1TX);
    GPIOPinTypeSSI(Neopixel_b, Neopixel_p);
    int k, i;
    for(k = 0; k < number; k++) {
        for(i=23; i >= 0; i--) {
            volatile uint8_t convert = 0xC0 ;//0b11000000;
            if((values[k] >> i) & 0x1 != 0) convert = 0xF8;// 0b11111000;
            SSIDataPut(SSI1_BASE, convert);
        }
    }
    GPIOPinTypeGPIOOutput(Neopixel_b, Neopixel_p);
    GPIOPinWrite(Neopixel_b, Neopixel_p, 0);
}


int main(void) {
    setupPins();
    setupInterrupts();

    // Latch Power TODO - Delay
    GPIOPinWrite(PowerON_b, PowerON_p, PowerON_p);

    ButtonLED_setSolid(100,100,100,100);
//    ButtonLED_setBlink(100,100,100,100, 100, 900);
//    ButtonLED_setPulse(0,40,80, 0, 100, 1000, 1000, 800, 700);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1)) {}

    GPIOPinConfigure(GPIO_PF1_SSI1TX);

    GPIOPinTypeSSI(Neopixel_b, Neopixel_p);
    SSIIntClear(SSI1_BASE,SSI_TXEOT);
    SSIConfigSetExpClk(SSI1_BASE, 80000000, SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER, 6400000, 8);
    SSIEnable(SSI1_BASE);

//    uint32_t grb = 0b000000001111111111111111;
//    uint32_t grb = 0b000000001111111100000000;
    uint32_t grb = 0b000000000000000011111111;
    int i;
    for(i = 0; i < 140; i++) {
        data[i] = grb;
    }




    while(1) {
        controlLoop();
        Neopixel_sendData(data, 140);

        SysCtlDelay(2000000);
    }
}
