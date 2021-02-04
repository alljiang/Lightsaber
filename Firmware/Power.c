
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

void Power_initialize() {
    GPIOPinTypeGPIOOutput(PowerON_b, PowerON_p);
    GPIOPinTypeGPIOOutput(Charge_Bias_b, Charge_Bias_p);
    GPIOPinTypeGPIOInput(Not_Charging_b, Not_Charging_p);

    // Initialize ADC0 module for Vsense, the battery voltage
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    // Wait for module to be ready.
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}
    // Enable first sample sequencer to capture the value of channel 2 when processor trigger occurs
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH2);
    ADCSequenceEnable(ADC0_BASE, 0);

}

void Power_latchOn() {
    GPIOPinWrite(PowerON_b, PowerON_p, PowerON_p);
}

void Power_latchOff() {
    // Goodbye cruel world
    GPIOPinWrite(PowerON_b, PowerON_p, 0);
}

// Return an int of the voltage of battery * 100
int Power_getBatteryVoltage() {
    uint32_t VsenseADC;

    // Trigger the sample sequence.
    ADCProcessorTrigger(ADC0_BASE, 0);
    while(!ADCIntStatus(ADC0_BASE, 0, false)) {}

    // Read the value from the ADC.
    ADCSequenceDataGet(ADC0_BASE, 0, &VsenseADC);

    // Return sample data divided by 2^12 bit resolution multiplied by logical high multiplied by voltage divider then scaled up
    return (VsenseADC * 33 * 2 * 10) / 4096;
}

int Power_isCharging() {
    /*
     *  BL4054B Charger IC
     *  Charging Mode: Strong pull down
     *  Standby Mode: Weak pull down
     */
}
