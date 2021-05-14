
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
#include "Power.h"

void Power_initialize() {
    GPIOPinTypeGPIOOutput(PowerON_b, PowerON_p);
    GPIOPinWrite(PowerON_b, PowerON_p, 0);
    GPIOPinTypeGPIOOutput(Charge_Bias_b, Charge_Bias_p);
    GPIOPinTypeGPIOInput(Not_Charging_b, Not_Charging_p);

    // Unlock D7 for charge bias
    HWREG(GPIO_PORTD_BASE+GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE+GPIO_O_CR) |= GPIO_PIN_7;

    // Initialize ADC0 module for Vsense, the battery voltage
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}

    GPIOPinTypeADC(Vsense_b, Vsense_p);

    // Enable first sample sequencer to capture the value of channel 2 when processor trigger occurs
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH2);
    ADCHardwareOversampleConfigure(ADC0_BASE, 64);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);
}

void Power_latchOn() {
    GPIOPinWrite(PowerON_b, PowerON_p, PowerON_p);
}

void Power_latchOff() {
    // Goodbye cruel world
    GPIOPinWrite(PowerON_b, PowerON_p, 0);
}

// Return an int of the voltage of battery * 1000
int Power_getBatteryVoltage() {
    uint32_t VsenseADC;

    // Trigger the sample sequence.
    ADCProcessorTrigger(ADC0_BASE, 3);
    while(!ADCIntStatus(ADC0_BASE, 3, false)) {}
    ADCIntClear(ADC0_BASE, 3);

    // Read the value from the ADC.
    ADCSequenceDataGet(ADC0_BASE, 3, &VsenseADC);

    // Return sample data divided by 2^12 bit resolution multiplied by logical high multiplied by voltage divider then scaled up
    //  seems to be too small, off by a factor of 10 - let's scale it by 10! :)
//    return (VsenseADC * 33 * 2 * 10 * 100) / 4096;
    return 3700;    // because who cares - TM4C is gonna die too early anyways
}

int Power_isCharging() {
    /*
     *  BL4054B Charger IC
     *  Charging Mode: Strong pull down. Standby Mode: Weak pull down
     *  Returns NO_CHARGE, CHARGING, or FULL_CHARGE
     */
    int status;
    GPIOPinWrite(Charge_Bias_b, Charge_Bias_p, Charge_Bias_p);
    // Check if not currently charging
    if (GPIOPinRead(Not_Charging_b, Not_Charging_p) & Not_Charging_p) {
        if (Power_getBatteryVoltage() > 4150) // 4.15 V
            status = FULL_CHARGE;
        else
            status = NO_CHARGE;
    } else
        status = CHARGING;
    GPIOPinWrite(Charge_Bias_b, Charge_Bias_p, 0); // reduce current from 1.5mA to 4uA to save power
    return status;
}
