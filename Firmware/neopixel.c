
#include <stdio.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"


void static commonInit(const uint8_t *cmdList) {
    volatile uint32_t delay;

    SYSCTL_RCGCSSI_R |= 0x01;  // activate SSI0
    SYSCTL_RCGCGPIO_R |= 0x01; // activate port A
    while((SYSCTL_PRGPIO_R&0x01)==0){}; // allow time for clock to start

    // toggle RST low to reset; CS low so it'll listen to us
    // SSI0Fss is temporarily used as GPIO
    GPIO_PORTA_DIR_R |= 0xC8;             // make PA3,6,7 out
    GPIO_PORTA_AFSEL_R &= ~0xC8;          // disable alt funct on PA3,6,7
    GPIO_PORTA_DEN_R |= 0xC8;             // enable digital I/O on PA3,6,7
                                        // configure PA3,6,7 as GPIO
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0x00FF0FFF)+0x00000000;
    GPIO_PORTA_AMSEL_R &= ~0xC8;          // disable analog functionality on PA3,6,7

    // initialize SSI0
    GPIO_PORTA_AFSEL_R |= 0x2C;           // enable alt funct on PA2,3,5
    GPIO_PORTA_DEN_R |= 0x2C;             // enable digital I/O on PA2,3,5
                                        // configure PA2,3,5 as SSI
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFF0F00FF)+0x00202200;
    GPIO_PORTA_AMSEL_R &= ~0x2C;          // disable analog functionality on PA2,3,5
    SSI0_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
    SSI0_CR1_R &= ~SSI_CR1_MS;            // master mode
                                        // configure for system clock/PLL baud clock source
    SSI0_CC_R = (SSI0_CC_R&~SSI_CC_CS_M)+SSI_CC_CS_SYSPLL;
    //                                        // clock divider for 3.125 MHz SSIClk (50 MHz PIOSC/16)
    //  SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M)+16;
                                        // clock divider for 8 MHz SSIClk (80 MHz PLL/24)
                                        // SysClk/(CPSDVSR*(1+SCR))
                                        // 80/(10*(1+0)) = 8 MHz (slower than 4 MHz)
    SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M)+4; // must be even number
    SSI0_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (8 Mbps data rate)
                  SSI_CR0_SPH |         // SPH = 0
                  SSI_CR0_SPO);         // SPO = 0
                                        // FRF = Freescale format
    SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO;
                                        // DSS = 8-bit data
    SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_DSS_M)+SSI_CR0_DSS_8;
    SSI0_CR1_R |= SSI_CR1_SSE;            // enable SSI
}
