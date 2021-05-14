/*
 * ESP.cpp
 *
 *  Created on: Apr 29, 2021
 *      Author: adeel
 */

#include "ESP.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/UART.h"
#include "driverlib/rom.h"
#include "tm4c123gh6pm.h"
#include "pinConfig.h"


bool ESP_SendCommand(char* str);
void UART4_OutString(char* str);
void UART4_OutCharArr(char* str, int len);
void UART4_Handler();
void ESP_EnableRXInts();
void ESP_DisableRXInts();

#ifdef ESP_DEBUG_MODE
void UART0_OutString(char* str);
void UART0_OutCharArr(char* str, int len);
#endif

#define ESP_RX_BUFFER_LENGTH 1000
volatile char ESP_RX_BUFFER[ESP_RX_BUFFER_LENGTH];

volatile char CONNECTION_ID;

#define ESP_RX_FIFO_LENGTH 12
#define ESP_RX_FIFO_WIDTH 48
volatile char ESP_RX_FIFO[ESP_RX_FIFO_LENGTH][ESP_RX_FIFO_WIDTH];
volatile int ESP_RX_FIFO_PutI = 0, ESP_RX_FIFO_GetI = 0;

char strSendOk[] = "\r\nSEND OK\r\n";
char strSendFail[] = "\r\nSEND FAIL\r\n";

/*
 * Initializes the ESP0
 * Flash Mode
 * Runs the program that is already uploaded
 * to the module
 *
 * WIFI Mode - AP
 */
void ESP_Init() {

    // Enable UART for the ESP
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART4));

    ROM_GPIOPinConfigure(GPIO_PC5_U4TX);
    ROM_GPIOPinConfigure(GPIO_PC4_U4RX);
    ROM_GPIOPinTypeUART(ESP_TX_b, ESP_TX_p | ESP_RX_p);

    UARTConfigSetExpClk(UART4_BASE, SysCtlClockGet(), 115200,
    (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
    UART_CONFIG_PAR_NONE));

    // Enable UART interrupts for Recieving Data
    UARTIntRegister(UART4_BASE, &UART4_Handler);
//    UART4_TIFLS_R &= ~(0x7<<3); // Trigger Interrupt when FIFO 1/8 Full

#ifdef ESP_DEBUG_MODE
    // Enable PC UART for Echo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
        UART_CONFIG_PAR_NONE));

    UART0_OutString("Welcome\r\n");

#endif

    // Start ESP in Flash Mode, Reset 0, CH Enable 1
    GPIOPinTypeGPIOOutput(ESP_IO0_b, ESP_IO0_p);
    GPIOPinTypeGPIOOutput(ESP_IO2_b, ESP_IO2_p);

    GPIOPinWrite(ESP_IO0_b, ESP_IO0_p, ESP_IO0_p);
    GPIOPinWrite(ESP_IO2_b, ESP_IO2_p, ESP_IO2_p);

    GPIOPinTypeGPIOOutput(ESP_RST_b, ESP_RST_p);
    GPIOPinTypeGPIOOutput(ESP_EN_b, ESP_EN_p);

    GPIOPinWrite(ESP_RST_b, ESP_RST_p, ESP_RST_p);
    GPIOPinWrite(ESP_EN_b, ESP_EN_p, 0);

    SysCtlDelay(8000000);
    GPIOPinWrite(ESP_EN_b, ESP_EN_p, ESP_EN_p);

    SysCtlDelay(8000000);
    GPIOPinWrite(ESP_RST_b, ESP_RST_p, 0);

    SysCtlDelay(8000000);
    GPIOPinWrite(ESP_RST_b, ESP_RST_p, ESP_RST_p);

    SysCtlDelay(8000000);

    for (int i=0; i<ESP_RX_BUFFER_LENGTH; i++) ESP_RX_BUFFER[i] = 0xFF;


    // Restart ESP (I didn't have this earlier and got some inconsistency around TM4C resets
//    ESP_SendCommand(AT_RESET);
//
//    int bI=0;
//    int OK;
//    do {
//        if (UARTCharsAvail(UART4_BASE)) {
//            ESP_RX_BUFFER[bI++] = UARTCharGetNonBlocking(UART4_BASE);
//        }
//
//        OK = (bI>=9 && bI<ESP_RX_BUFFER_LENGTH) ?
//                !strncmp(&ESP_RX_BUFFER[bI-9], "\r\nready\r\n", 9) : false;
//
//    } while(!OK);

    ESP_EnableRXInts();


    // Configure ESP
    ESP_SendCommand(AT_Disable_Echo);       // Disable UART Echo
    ESP_SendCommand(AT_SET_WIFI_AP);        // Config as Access Point
    ESP_SendCommand(AT_AP_CONFIG);          // Set SSID and Password
    ESP_SendCommand(AT_DHCP_EN);            // Enable DHCP Server
    ESP_SendCommand(AT_SAP_IP_SET);         // Set Server IP to 192.168.4.1
    ESP_SendCommand(AT_MULTI_EN);           // Enable Multiple Connections
    ESP_SendCommand(AT_SERVER_CONFIG);      // Enable Server on Port 333
    ESP_SendCommand(AT_TRANSFER_NORMAL);    // Enable Transparent Transmission
    ESP_SendCommand(AT_LONG_TIMEOUT);       // 2 hour timeout so it doesn't DC.

//    ESP_SendString("Hello\0World", 12);
    ESP_EnableRXInts();
}

/*
 * Tests the ESP using AT command
 * Returns - True if ESP working
 */
bool ESP_Test() {
    return ESP_SendCommand(AT_Test);
}

/*
 *  Waits for client to connect to server
 *  Busy wait
 */
void ESP_Connect() {
    ESP_DisableRXInts();

    do {
        if (ESP_SendCommand("AT+CIPSEND=0,11\r\n")) {
            CONNECTION_ID = '0';
            break;
        }
        if (ESP_SendCommand("AT+CIPSEND=1,11\r\n")) {
            CONNECTION_ID = '1';
            break;
        }
        if (ESP_SendCommand("AT+CIPSEND=2,11\r\n")) {
            CONNECTION_ID = '2';
            break;
        }
        if (ESP_SendCommand("AT+CIPSEND=3,11\r\n")) {
            CONNECTION_ID = '3';
            break;
        }

    } while(true);

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Sending:\r\n");
    UART0_OutString("Connected\r\n");
    UART0_OutString("Done\r\n\n");
#endif

    UART4_OutString("Connected\r\n");

    int bI=0;
    int OK, ERROR;
    do {
        if (UARTCharsAvail(UART4_BASE)) {
            ESP_RX_BUFFER[bI++] = UARTCharGetNonBlocking(UART4_BASE);
        }

        OK = (bI>=11 && bI<ESP_RX_BUFFER_LENGTH) ?
                !strncmp(&ESP_RX_BUFFER[bI-11], strSendOk, 11) : false;

        ERROR = (bI>=13 && bI<ESP_RX_BUFFER_LENGTH) ?
                        !strncmp(&ESP_RX_BUFFER[bI-13], strSendFail, 13) : false;

    } while(!OK && !ERROR);
    ESP_RX_BUFFER[bI] = 0;
    ESP_EnableRXInts();

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Response:\r\n");
    UART0_OutString(ESP_RX_BUFFER);
    UART0_OutString("Done\r\n\n");

    for (int i=0; i<ESP_RX_BUFFER_LENGTH; i++) ESP_RX_BUFFER[i] = 0xFF;
#endif

}

/*
 * Checks if client is connected to server
 * Opens TCP stream with client if not already open
 */
bool ESP_TryConnect() {
    ESP_DisableRXInts();

    if (CONNECTION_ID != 0 && ESP_SendString("Connected?\r\n", 12)) {
        return true;
    }

    int connected = false;
    do {
        if (ESP_SendCommand("AT+CIPSEND=0,11\r\n")) {
            CONNECTION_ID = '0';
            connected = true;
            break;
        }
        if (ESP_SendCommand("AT+CIPSEND=1,11\r\n")) {
            CONNECTION_ID = '1';
            connected = true;
            break;
        }
        if (ESP_SendCommand("AT+CIPSEND=2,11\r\n")) {
            CONNECTION_ID = '2';
            connected = true;
            break;
        }
        if (ESP_SendCommand("AT+CIPSEND=3,11\r\n")) {
            CONNECTION_ID = '3';
            connected = true;
            break;
        }

    } while(false);

    if (!connected) {
        ESP_EnableRXInts();
        return false;
    }


    UART4_OutString("Connected\r\n");

    int bI=0;
    int OK, ERROR;
    do {
        if (UARTCharsAvail(UART4_BASE)) {
            ESP_RX_BUFFER[bI++] = UARTCharGetNonBlocking(UART4_BASE);
        }

        OK = (bI>=11 && bI<ESP_RX_BUFFER_LENGTH) ?
                !strncmp(&ESP_RX_BUFFER[bI-11], strSendOk, 11) : false;

        ERROR = (bI>=13 && bI<ESP_RX_BUFFER_LENGTH) ?
                        !strncmp(&ESP_RX_BUFFER[bI-13], strSendFail, 13) : false;

    } while(!OK && !ERROR);
    ESP_RX_BUFFER[bI] = 0;

    ESP_EnableRXInts();

    return true;
}

/*
 * Sends a single byte through TCP stream
 * Must be connected already
 * Call ESP COnnect to connect
 * Returns - True if successfully sent
 */
char sendcharcmd[] = "AT+CIPSEND=_,1\r\n";
bool ESP_SendChar(char c) {

    ESP_DisableRXInts();

    sendcharcmd[11] = CONNECTION_ID;

    if (!ESP_SendCommand(sendcharcmd)) return false;

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Sending:\r\n");
    UART0_OutCharArr(&c, 1);
    UART0_OutString("Done\r\n\n");
#endif

    UARTCharPut(UART4_BASE, c);

    int bI=0;
    int OK, ERROR;
    do {
        if (UARTCharsAvail(UART4_BASE)) {
            ESP_RX_BUFFER[bI++] = UARTCharGetNonBlocking(UART4_BASE);
        }

        OK = (bI>=11 && bI<ESP_RX_BUFFER_LENGTH) ?
                !strncmp(&ESP_RX_BUFFER[bI-11], strSendOk, 11) : false;

        ERROR = (bI>=13 && bI<ESP_RX_BUFFER_LENGTH) ?
                        !strncmp(&ESP_RX_BUFFER[bI-13], strSendFail, 13) : false;

    } while(!OK && !ERROR);
    ESP_RX_BUFFER[bI] = 0;
    ESP_EnableRXInts();

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Response:\r\n");
    UART0_OutString(ESP_RX_BUFFER);
    UART0_OutString("Done\r\n\n");

    for (int i=0; i<ESP_RX_BUFFER_LENGTH; i++) ESP_RX_BUFFER[i] = 0xFF;
#endif

    return OK;
}

/*
 *  Sends a byte array through TCP stream
 *  Must be connected already
 *  Call ESP_Connect to connect.
 *  Returns - True if successfully sent
 *
 *  Sends as StupidString if it contains a \r
 */
bool ESP_SendString(char* str, int len) {

    for (int i=0; i<len; i++) {
        if (str[i] == '\r' || str[i] == '\n' || str[i] == 0) {
            return ESP_SendStupidString(str, len);
        }
    }

    ESP_DisableRXInts();

    char cmd[] = "AT+CIPSEND=_,__\r\n";
    cmd[11] = CONNECTION_ID;
    cmd[13] = len/10 + '0';
    cmd[14] = len%10 + '0';

    if (!ESP_SendCommand(cmd)) {
        return false;
    }

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Sending:\r\n");
    UART0_OutCharArr(str, len);
    UART0_OutString("Done\r\n\n");
#endif

    UART4_OutCharArr(str, len);

    int bI=0;
    int OK, ERROR;
    do {
        if (UARTCharsAvail(UART4_BASE)) {
            ESP_RX_BUFFER[bI++] = UARTCharGetNonBlocking(UART4_BASE);
        }

        OK = (bI>=11 && bI<ESP_RX_BUFFER_LENGTH) ?
                !strncmp(&ESP_RX_BUFFER[bI-11], strSendOk, 11) : false;

        ERROR = (bI>=13 && bI<ESP_RX_BUFFER_LENGTH) ?
                        !strncmp(&ESP_RX_BUFFER[bI-13], strSendFail, 13) : false;

    } while(!OK && !ERROR);
    ESP_RX_BUFFER[bI] = 0;
    ESP_EnableRXInts();

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Response:\r\n");
    UART0_OutString(ESP_RX_BUFFER);
    UART0_OutString("Done\r\n\n");

    for (int i=0; i<ESP_RX_BUFFER_LENGTH; i++) ESP_RX_BUFFER[i] = 0xFF;
#endif

    return OK;

}

/*
 *  Like Send String but sends each char
 *  as an individual packet
 */
bool ESP_SendStupidString(char* str, int len) {
    bool r = false;
    for (int i=0; i<len; i++) {
        r |= !ESP_SendChar(str[i]);
    }
    return !r;
}

char strOK[] = "\r\nOK\r\n";
char strERROR[] = "\r\nERROR\r\n";

/*
 * Sends a command to the ESP
 * Places response in RX Buffer
 * In debug mode, prints task to UART0
 * Returns - True if No Error
 */
bool ESP_SendCommand(char* command) {

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Sending:\r\n");
    UART0_OutString(command);
    UART0_OutString("Done\r\n\n");
#endif

//    ESP_DisableRXInts();
    UART4_OutString(command);
    int bI=0;
    int OK, ERROR;
    do {
        if (UARTCharsAvail(UART4_BASE)) {
            ESP_RX_BUFFER[bI++] = UARTCharGetNonBlocking(UART4_BASE);
        }

        OK = (bI>=6 && bI<ESP_RX_BUFFER_LENGTH) ?
                !strncmp(&ESP_RX_BUFFER[bI-6], strOK, 6) : false;

        ERROR = (bI>=9 && bI<ESP_RX_BUFFER_LENGTH) ?
                        !strncmp(&ESP_RX_BUFFER[bI-9], strERROR, 9) : false;

    } while(!OK && !ERROR);
    ESP_RX_BUFFER[bI] = 0;
//    ESP_EnableRXInts();

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Response:\r\n");
    UART0_OutString(ESP_RX_BUFFER);
    UART0_OutString("Done\r\n\n");

    for (int i=0; i<ESP_RX_BUFFER_LENGTH; i++) ESP_RX_BUFFER[i] = 0xFF;
#endif

    return (bool)OK;

}

/*
 * Retrieves a byte array from RX_FIFO (Connected to TCP Stream)
 * Length - Place to put length of array
 * Returns - Pointer to char array if successful, 0 else
 */
char* ESP_GetString(int* length) {
    if (!ESP_StringAvail()) {
        *length = 0;
        return 0;
    }

    *length = ESP_RX_FIFO[ESP_RX_FIFO_GetI][0];
    char* toReturn = ESP_RX_FIFO[ESP_RX_FIFO_GetI] + 1;
    ESP_RX_FIFO_GetI = (ESP_RX_FIFO_GetI+1)%ESP_RX_FIFO_LENGTH;

    return toReturn;
}

/*
 * Returns true if there is data ready for reading in
 * the Recieve Buffer
 */
volatile bool ESP_StringAvail() {
    return ESP_RX_FIFO_PutI != ESP_RX_FIFO_GetI;
}

/*
 * Reads data from UART Buffer and chucks it into the Recieve FIFO.
 * First element of buffer is length
 */
void UART4_Handler() {
    UARTIntClear(UART4_BASE, UART_INT_RX);

    int i = -1;
    char cmd[16];

    do {
        cmd[++i] = UARTCharGet(UART4_BASE);
    } while(i<16 && cmd[i] != ':' && !(i>4 && cmd[i-1]=='\r' && cmd[i]=='\n'));

    // Make sure colon exists, not sure why it wouldn't
    if (cmd[i] != ':') {
        return;
    }

    // Make sure we have a +IPD command
    // Toss all other commands, idk if they even exist lol
    if (strncmp(cmd, "\r\n+IPD,", 7) != 0) {
        return;
    }

    // Make sure epxected client is the one sending packets
    // No funny business, like Allen trying to hack Sophia's Saber
    if (cmd[7] != CONNECTION_ID) {
        return;
    }

    int len = 0;
    for (int j=9; j<i; j++) {
        len = len*10 + (cmd[j] - '0');
    }

    // Check if Space in FIFO, read and toss packet if not.
    if ((ESP_RX_FIFO_PutI+1)%ESP_RX_FIFO_LENGTH == ESP_RX_FIFO_GetI) {
        while(len--) {
            UARTCharGet(UART4_BASE);
        }
        return;
    }

    ESP_RX_FIFO[ESP_RX_FIFO_PutI][0] = len;

    int k = 1;
    while(k<=len) {
        ESP_RX_FIFO[ESP_RX_FIFO_PutI][k++] = UARTCharGet(UART4_BASE);
    }

    ESP_RX_FIFO[ESP_RX_FIFO_PutI][k] = 0;

#ifdef ESP_DEBUG_MODE
    UART0_OutString("Recieving Data\r\n");
    UART0_OutString(ESP_RX_FIFO[ESP_RX_FIFO_PutI]+1);
    UART0_OutString("Done\r\n\n");
#endif

    ESP_RX_FIFO_PutI++;
    ESP_RX_FIFO_PutI %= ESP_RX_FIFO_LENGTH;
}

void ESP_EnableRXInts() {
    while(UARTCharsAvail(UART4_BASE)) {
        UARTCharGetNonBlocking(UART4_BASE);
    }
    UARTIntClear(UART4_BASE, UART_INT_RX);
    UARTIntEnable(UART4_BASE, UART_INT_RX);
}

void ESP_DisableRXInts() {
    while(UARTCharsAvail(UART4_BASE)) {
        UARTCharGetNonBlocking(UART4_BASE);
    }
    UARTIntClear(UART4_BASE, UART_INT_RX);
    UARTIntDisable(UART4_BASE, UART_INT_RX);
}

void UART4_OutString(char* str) {
    int i=0;
    while(str[i]) {
        UARTCharPut(UART4_BASE, str[i++]);
    }
}

void UART4_OutCharArr(char* str, int len) {
    for (int i=0; i<len; i++) {
        UARTCharPut(UART4_BASE, str[i++]);
    }
}

#ifdef ESP_DEBUG_MODE

void UART0_OutString(char* str) {
    int i=0;
    while(str[i]) {
        UARTCharPut(UART0_BASE, str[i++]);
    }
}

void UART0_OutCharArr(char* str, int len) {
    int i=0;
    while(i<len) {
        UARTCharPut(UART0_BASE, str[i++]);
    }
}

#endif
