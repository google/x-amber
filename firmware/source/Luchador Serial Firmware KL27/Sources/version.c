/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * version.c
 *
 *  Created on: Nov 2, 2018
 *      Author: nick
 */

#include "version.h"
//#include "Flash.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "HardwareVersion.h"

//#define TESTIFR

#define FLASHCMD_READONCE		(0x41)
#define FLASHCMD_PROGRAMONCE	(0x43)



//	UPDATE THESE VALUES WHEN RELEASING A NEW VERSION
static const uint32_t MAJOR=1;
static const uint32_t MINOR=6;
static const uint32_t PATCH=2;

static volatile bool flashDone=false;



static void ReadSerialFromIfr(char *serial);
static bool ProgramSerialToIfr(char *serial);

uint32_t Version_GetMajor()
{
	return MAJOR;
}

uint32_t Version_GetMinor()
{
	return MINOR;
}
uint32_t Version_GetPatch()
{
	return PATCH;
}

void Version_GetVersionString(char *version)
{
	sprintf(version, "%lu.%lu.%lu", MAJOR, MINOR, PATCH);
}


const char* Version_GetBuildNumber()
{
	extern char* BUILDNUMBER;
	return BUILDNUMBER;

}

void Version_GetHwVersionString(char *version)
{


	uint32_t ver= (GPIO_DRV_ReadPinInput(GPIO_HWV3)<<3) + (GPIO_DRV_ReadPinInput(GPIO_HWV2)<<2) + (GPIO_DRV_ReadPinInput(GPIO_HWV1)<<1) + GPIO_DRV_ReadPinInput(GPIO_HWV0);


	switch(ver)
	{
	case 0xf:
		strcpy(version, "3.0");
		break;

	case 0xe:
		strcpy(version, "3.1");
		break;

	default:
		strcpy(version, "UNKNOWN");
	}

}

//	Gets the serial number, 32 bytes
void Version_GetSerialNumber(char *serial)
{
	ReadSerialFromIfr(serial);
	volatile uint8_t chr=(uint8_t)*serial;
	if(chr==0xff)
	{
		strcpy(serial, "NOT_SET");
	}
}

//	Sets the serial number.  Will fail if serial number already set
bool Version_SetSerialNumber(char *serial)
{
	char serialPad[SERIAL_SIZE];
	ReadSerialFromIfr(serialPad);
	uint8_t chr=(uint8_t)serialPad[0];
	if(chr!=0xff) return false;
	memset(serialPad, 0, SERIAL_SIZE);
	strcpy(serialPad,serial);
	return ProgramSerialToIfr(serialPad);

}

//	This will always read 32 bytes beginning at the first record (0)
static void ReadSerialFromIfr(char *serial)
{
	//	Disable interrupts
	__disable_irq();
	for(int record=0;record<8;record++)
	{
		FTFA_FCCOB0=FLASHCMD_READONCE;
		FTFA_FCCOB1=record;
		FTFA_FSTAT=0x30;	//	Clear any errors

		//	Start read
		FTFA_FSTAT |= FTFA_FSTAT_CCIF_MASK;
		while(!(FTFA_FSTAT & FTFA_FSTAT_CCIF_MASK));	//	Wait for completion
		*serial++=FTFA_FCCOB4;
		*serial++=FTFA_FCCOB5;
		*serial++=FTFA_FCCOB6;
		*serial++=FTFA_FCCOB7;
	}
	__enable_irq();

}

//	Will write 32 bytes from serial to the program once register.  *Serial must be at least 32 bytes long
static bool ProgramSerialToIfr(char *serial)
{
	//	Disable interrupts
	__disable_irq();
	for(int record=0;record<8;record++)
	{
		FTFA_FCCOB0=FLASHCMD_PROGRAMONCE;
		FTFA_FCCOB1=record;
		FTFA_FSTAT=0x30;	//	Clear any errors

		FTFA_FCCOB4=*serial++;
		FTFA_FCCOB5=*serial++;
		FTFA_FCCOB6=*serial++;
		FTFA_FCCOB7=*serial++;

#ifndef TESTIFR
		//	Start read
		FTFA_FSTAT |= FTFA_FSTAT_CCIF_MASK;
		while(!(FTFA_FSTAT & FTFA_FSTAT_CCIF_MASK));			//	Wait for completion
		if(FTFA_FSTAT!=0x80)
		{
			//	Enable irq and return false if error condition
			__enable_irq();
			return false;
		}
#endif
	}
	__enable_irq();
	return true;
}

