/*
 * SDCard.h
 *
 *  Created on: Apr 1, 2021
 *      Author: Allen
 */

#ifndef SDCARD_H_
#define SDCARD_H_


void SDCard_init();

void SDCard_openFile(char* filename);
void SDCard_closeFile();

char* SDCard_read(uint32_t size);


#endif /* SDCARD_H_ */
