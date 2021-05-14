/*
 * ESP.h
 *
 *  Created on: Apr 29, 2021
 *      Author: adeel
 */

#ifndef ESP_H_
#define ESP_H_

//#define ESP_DEBUG_MODE

#include "stdint.h"
#include "stdbool.h"


#define AT_RESET "AT+RST\r\n"
#define AT_Test "AT\r\n"
#define AT_Version "AT+GMR\r\n"
#define AT_Disable_Echo "ATE0\r\n"
#define AT_SET_WIFI_AP "AT+CWMODE=2\r\n"
#define AT_AP_CONFIG "AT+CWSAP=\"Light Saber\",\"\",5,0\r\n"
#define AT_DHCP_EN "AT+CWDHCP=0,0\r\n"
#define AT_MULTI_EN "AT+CIPMUX=1\r\n"
#define AT_SERVER_CONFIG "AT+CIPSERVER=1\r\n"
#define AT_SAP_IP_GET "AT+CIPAP?\r\n"
#define AT_SAP_IP_SET "AT+CIPAP=\"192.168.4.1\"\r\n"
#define AT_TRANSFER_NORMAL "AT+CIPMODE=0\r\n"
#define AT_LIST_CLIENTS "AT+CWLIF\r\n"
#define AT_LONG_TIMEOUT "AT+CIPSTO=0\r\n"


/*
 * Initializes the ESP0
 * Flash Mode
 * Runs the program that is already uploaded
 * to the module
 *
 * Echo Off
 * WIFI Mode - AP
 */
void ESP_Init();

/*
 * Tests the ESP using AT command
 * Returns - True if ESP working
 */
bool ESP_Test();

/*
 *  Waits for client to connect to server
 *  Busy wait
 *  Opens TCP stream with client
 */
void ESP_Connect();

/*
 * Checks if client is connected to server
 * Attempts to connect if not connected already
 * Like ESP_Connect, but without Busy Wait. Returns false if not
 * instantly successful
 * Returns true if already connected or successful in connecting
 * Opens TCP stream with client
 */
bool ESP_TryConnect();

/*
 * Sends a single byte through TCP stream
 * Must be connected already
 * Call ESP COnnect to connect
 * Returns - True if successfully sent
 */
bool ESP_SendChar(char c);

/*
 *  Sends a byte array through TCP stream
 *  Must be connected already
 *  Call ESP_Connect to ensure it's connected.
 *  Params - Char Array to be sent, Length of Array
 *  Returns - True if successfully sent
 */
bool ESP_SendString(char* str, int len);

/*
 *  Like Send String but sends each char
 *  as an individual packet
 */
bool ESP_SendStupidString(char* str, int len);

/*
 * Retrieves a byte array from RX_FIFO (Connected to TCP Stream)
 * Length - Place to put length of array
 * Returns - Pointer to char array if successful, 0 else
 */
char* ESP_GetString(int* length);

/*
 * Returns true if there is data ready for reading in
 * the Recieve Buffer
 */
bool ESP_StringAvail();


/*
 * We're not gonna talk about this shit
 */
#ifdef ESP_DEBUG_MODE

void UART0_OutString(char* str);

void UART0_OutCharArr(char* str, int len);

#endif


#endif /* ESP_H_ */
