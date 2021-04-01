/*
 * ESP.h
 *
 *  Created on: Apr 1, 2021
 *      Author: Allen
 */

#ifndef ESP_H_
#define ESP_H_


void ESP_init();
void ESP_startAP();
void ESP_stopAP();

void ESP_send(char* buffer);


#endif /* ESP_H_ */
