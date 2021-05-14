
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
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
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "pinConfig.h"
#include "tm4c123gh6pm.h"
#include "TimeHandler.h"

#define BUTTON_LED_OFF -2
#define BUTTON_LED_IDLE -1
#define BUTTON_LED_SOLID 0
#define BUTTON_LED_BLINK 1
#define BUTTON_LED_PULSE 2
#define BUTTON_LED_RAINBOW 3

int8_t CONFIG_BUTTON_LED_MODE = 0;

uint8_t CONFIG_BUTTON_LED_SOLID_R = 0;
uint8_t CONFIG_BUTTON_LED_SOLID_G = 0;
uint8_t CONFIG_BUTTON_LED_SOLID_B = 0;
uint8_t CONFIG_BUTTON_LED_SOLID_A = 0;

uint8_t CONFIG_BUTTON_LED_BLINK_R = 0;
uint8_t CONFIG_BUTTON_LED_BLINK_G = 0;
uint8_t CONFIG_BUTTON_LED_BLINK_B = 0;
uint8_t CONFIG_BUTTON_LED_BLINK_A = 0;
uint32_t CONFIG_BUTTON_LED_BLINK_ONPERIOD = 0;
uint32_t CONFIG_BUTTON_LED_BLINK_OFFPERIOD = 0;

uint8_t CONFIG_BUTTON_LED_PULSE_R = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_G = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_B = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_LOWALPHA = 0;
uint8_t CONFIG_BUTTON_LED_PULSE_HIGHALPHA = 0;
uint32_t CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD = 0;
uint32_t CONFIG_BUTTON_LED_PULSE_TOPPERIOD = 0;
uint32_t CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD= 0;
uint32_t CONFIG_BUTTON_LED_PULSE_BOTTOMPERIOD = 0;

bool blink_isOn = false;
long blink_lastToggleMillis = 0;

int pulse_state = 0;
long pulse_lastStateMillis = 0;

void Button_initialize() {
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

    // Initialize ADC1 module for Button_sense, the button voltage
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)) {}

    // Configure ADC Pin
    GPIOPinTypeADC(Button_Sense_b, Button_Sense_p);

    // Enable first sample sequencer to capture the value of channel 3 when processor trigger occurs
    ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC1_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH3);

    ADCHardwareOversampleConfigure(ADC1_BASE, 32);

    ADCSequenceEnable(ADC1_BASE, 0);
    ADCIntClear(ADC1_BASE, 0);

}

void Button_rgb(int redPercent, int greenPercent, int bluePercent, int brightnessPercent) {
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

    PWM0_INVERT_R |= 0b1001000;
    PWMOutputInvert(PWM0_BASE, PWM_OUT_6 | PWM_OUT_5, true);
    PWMOutputInvert(PWM0_BASE, PWM_OUT_3, false);

    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT | PWM_OUT_5_BIT | PWM_OUT_6_BIT, true);
}

void Button_setSolid(int r, int g, int b, int alpha) {
    CONFIG_BUTTON_LED_MODE = BUTTON_LED_SOLID;

    CONFIG_BUTTON_LED_SOLID_R = r;
    CONFIG_BUTTON_LED_SOLID_G = g;
    CONFIG_BUTTON_LED_SOLID_B = b;
    CONFIG_BUTTON_LED_SOLID_A = alpha;
}

void Button_setBlink(int r, int g, int b, int alpha, int onPeriod, int offPeriod) {
    CONFIG_BUTTON_LED_MODE = BUTTON_LED_BLINK;

    CONFIG_BUTTON_LED_BLINK_R = r;
    CONFIG_BUTTON_LED_BLINK_G = g;
    CONFIG_BUTTON_LED_BLINK_B = b;
    CONFIG_BUTTON_LED_BLINK_A = alpha;
    CONFIG_BUTTON_LED_BLINK_ONPERIOD = onPeriod;
    CONFIG_BUTTON_LED_BLINK_OFFPERIOD = offPeriod;

    blink_isOn = false;
    blink_lastToggleMillis = 0;
}

void Button_setPulse(int r, int g, int b, int lowAlpha, int highAlpha, int rampUpPeriod, int topPeriod, int rampDownPeriod, int bottomPeriod) {
    CONFIG_BUTTON_LED_MODE = BUTTON_LED_PULSE;

    CONFIG_BUTTON_LED_PULSE_R = r;
    CONFIG_BUTTON_LED_PULSE_G = g;
    CONFIG_BUTTON_LED_PULSE_B = b;
    CONFIG_BUTTON_LED_PULSE_LOWALPHA = lowAlpha;
    CONFIG_BUTTON_LED_PULSE_HIGHALPHA = highAlpha;
    CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD = rampUpPeriod;
    CONFIG_BUTTON_LED_PULSE_TOPPERIOD = topPeriod;
    CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD= rampDownPeriod;
    CONFIG_BUTTON_LED_PULSE_BOTTOMPERIOD = bottomPeriod;

    pulse_state = 0;
    pulse_lastStateMillis = millis();
}

void Button_Handler() {

    uint32_t time = millis();

    if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_IDLE) {}
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_OFF) {
        Button_rgb(0,0,0,0);
        CONFIG_BUTTON_LED_MODE = BUTTON_LED_IDLE;
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_SOLID) {
        Button_rgb(CONFIG_BUTTON_LED_SOLID_R, CONFIG_BUTTON_LED_SOLID_G, CONFIG_BUTTON_LED_SOLID_B, CONFIG_BUTTON_LED_SOLID_A);
        CONFIG_BUTTON_LED_MODE = BUTTON_LED_IDLE;
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_BLINK) {
        if(blink_isOn) {
            if(time - blink_lastToggleMillis >= CONFIG_BUTTON_LED_BLINK_ONPERIOD) {
                //  turn off
                Button_rgb(0,0,0,0);
                blink_lastToggleMillis = time;
                blink_isOn = false;
            }
        } else {
            if(time - blink_lastToggleMillis >= CONFIG_BUTTON_LED_BLINK_OFFPERIOD) {
                //  turn on
                Button_rgb(CONFIG_BUTTON_LED_BLINK_R, CONFIG_BUTTON_LED_BLINK_G, CONFIG_BUTTON_LED_BLINK_B, CONFIG_BUTTON_LED_BLINK_A);
                blink_lastToggleMillis = time;
                blink_isOn = true;
            }
        }
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_PULSE) {
        if(pulse_state == 0) {
            //  bottom
            if(time >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_BOTTOMPERIOD) {
                pulse_state = 1;
                pulse_lastStateMillis = time;
            }
            else {
                Button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B, CONFIG_BUTTON_LED_PULSE_LOWALPHA);
            }
        }
        if(pulse_state == 1) {
            //  ramp up
            if(time >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD) {
                pulse_state = 2;
                pulse_lastStateMillis = time;
            }
            else {
                uint8_t alpha = (CONFIG_BUTTON_LED_PULSE_HIGHALPHA - CONFIG_BUTTON_LED_PULSE_LOWALPHA) * (time - pulse_lastStateMillis) / CONFIG_BUTTON_LED_PULSE_RAMPUPPERIOD + CONFIG_BUTTON_LED_PULSE_LOWALPHA;
                Button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B,  alpha);
            }
        }
        if(pulse_state == 2) {
            //  top
            if(time >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_TOPPERIOD) {
                pulse_state = 3;
                pulse_lastStateMillis = time;
            }
            else {
                Button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B, CONFIG_BUTTON_LED_PULSE_HIGHALPHA);
            }
        }
        if(pulse_state == 3) {
            //  ramp down
            if(time >= pulse_lastStateMillis + CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD) {
                pulse_state = 0;
                pulse_lastStateMillis = time;
            }
            else {
                uint8_t alpha = CONFIG_BUTTON_LED_PULSE_HIGHALPHA - (CONFIG_BUTTON_LED_PULSE_HIGHALPHA - CONFIG_BUTTON_LED_PULSE_LOWALPHA) * (time - pulse_lastStateMillis) / CONFIG_BUTTON_LED_PULSE_RAMPDOWNPERIOD;
                Button_rgb(CONFIG_BUTTON_LED_PULSE_R, CONFIG_BUTTON_LED_PULSE_G, CONFIG_BUTTON_LED_PULSE_B,  alpha);
            }
        }
    }
    else if(CONFIG_BUTTON_LED_MODE == BUTTON_LED_RAINBOW) {

    }

}

// Return true if button pressed and false otherwise
int Button_isPressed(void) {
    uint32_t ButtonSenseADC;
//
//    // Trigger the sample sequence.
    ADCProcessorTrigger(ADC1_BASE, 0);

    // Wait until the sample sequence has completed.
    while(!ADCIntStatus(ADC1_BASE, 0, false)) {}
    ADCIntClear(ADC1_BASE, 0);

    // Read the value from the ADC.
    ADCSequenceDataGet(ADC1_BASE, 0, &ButtonSenseADC);

    // ADC samples around 470 when button is pressed and close to 0 otherwise
    if (ButtonSenseADC > 200 && ButtonSenseADC < 900)
        return 1;
    else
        return 0;
}
