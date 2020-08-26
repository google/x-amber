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
 * eeg.c
 *
 *  Created on: Sep 29, 2016
 *      Author: nick
 */

#include "EEG.H"
#include "utils.h"
#include "spi.h"
#include "Serial/serial.h"
#include "settings.h"
#include "GpioEeg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define SIMDATARATE		(4)

volatile int32_t eegData[SAMPSIZE][32];
volatile uint32_t eegStatus[SAMPSIZE][4];
volatile int pPtr=0;
volatile int cPtr=0;
volatile int smpPtr=0;
volatile bool drdy=0;
volatile uint16_t tmr_EEGSIM;
bool updateRegisterFlag=false;

uint8_t ADS_Shadow[MODULES*REGISTERS];
uint8_t ADS_TempRegisters[MODULES*REGISTERS];


void Serial_SetRegister(int reg, int value)
{
	ADS_TempRegisters[reg]=value;
	updateRegisterFlag=true;
}

void Init_EEG(void)
{

}


void Reset_EEG(void)
{

#if SET_SYNC==1
	Delay(500);
	GPIO_DRV_ClearPinOutput(GPIO_RESET);
	GPIO_DRV_ClearPinOutput(GPIO_PWDN);
	GPIO_DRV_ClearPinOutput(GPIO_START);
	Delay(500);
	GPIO_DRV_SetPinOutput(GPIO_RESET);
	GPIO_DRV_SetPinOutput(GPIO_PWDN);
	Delay(500);
	GPIO_DRV_ClearPinOutput(GPIO_RESET);
	Delay(500);
	GPIO_DRV_SetPinOutput(GPIO_RESET);
	Delay(500);
	StopRDCMode();
	//EnterRegisterMode();
	//	Set the registers on the first adc
	WriteRegister(REG_CONFIG1,0xB6,0x01);		//	Turn on the clock output
	Delay(500);
	StopRDCMode();
	//	Reset the other ADCs
	Spi_SendReset(0x0e);
	Delay(500);
	StopRDCMode();
	//	Write the registers on the other ADCs
	WriteRegister(REG_CONFIG1,0x96, 0x0e);

	//	Now write the other 2 registers
	WriteRegister(REG_CONFIG2, 0b11010000, 0x0f);
	WriteRegister(REG_CONFIG3, 0b11101100, 0x0f);
#else
	Delay(500);
	ADS_RESET_ClrVal(ads_reset);
	ADS_PWDN_ClrVal(ads_pwdn);
	ADS_START_ClrVal(ads_start);
	Delay(500);
	ADS_RESET_SetVal(ads_reset);
	ADS_PWDN_SetVal(ads_pwdn);
	Delay(500);
	ADS_RESET_ClrVal(ads_reset);
	Delay(500);
	ADS_RESET_SetVal(ads_reset);
	Delay(500);
	StopRDCMode();

	WriteRegister(REG_CONFIG1,0b11010110,0x0f);
	WriteRegister(REG_CONFIG2,0b11010000, 0x0f);
	WriteRegister(REG_CONFIG3,0b11101100, 0x0f);
	WriteRegister(REG_LOFF, 0b00000000, 0x0f);

#endif




}

void Do_EEG(void)
{
	extern bool converting;

	if(drdy && converting)
	{
		ReadEEG();
		drdy=0;
	}
}

void SimulateEEG(void)
{
	if(tmr_EEGSIM==0)
	{
		tmr_EEGSIM=SIMDATARATE;

		for(uint32_t x=0;x<32;x++)
		{
			eegData[pPtr][x]=(-10+x)&0x00ffffff;
		}

		pPtr++;
		if(pPtr==SAMPSIZE)
		{
			//eegDataRdy=1;
			pPtr=0;
		}
	}
}

void CopyToTempRegisters(uint8_t *data)
{
	int ro;	//	Register offset
	for(int module=0;module<MODULES;module++)
	{
		for(int reg=0;reg<REGISTERS;reg++)
		{
			ro=module*0x18+reg;
			ADS_TempRegisters[ro]=data[ro];
		}
	}
	updateRegisterFlag=true;
}

void WriteChangedRegisters(void)
{
	int ro;	//	Register offset
	for(int module=0;module<MODULES;module++)
	{
		for(int reg=0;reg<REGISTERS;reg++)
		{
			ro=module*REGISTERS+reg;
			//	Check for fixed bits that must be set exactly
			if(reg==REG_ID) 	//	ID
			{
				ADS_TempRegisters[ro]|=0x01;
			}
			else if(reg==REG_CONFIG1)	//	Config1
			{
#if SET_SYNC==1
				if(module==0) ADS_TempRegisters[ro]=0b11110110;
				else ADS_TempRegisters[ro]=0b11010110;
#else
				ADS_TempRegisters[ro]=0b11010110;

#endif
				//ADS_TempRegisters[ro]&=~(0x08);
			}
			else if(reg==REG_CONFIG2)	//	Config2
			{
				ADS_TempRegisters[ro]|=0xc0;
				ADS_TempRegisters[ro]&=~(0x24);
			}
			else if(reg==REG_CONFIG3)	//	Config3
			{
				ADS_TempRegisters[ro]|=0b11100000;	//	Make sure internal ref buf stays on
			}
			else if(reg==REG_LOFF)		//	LOFF
			{
				ADS_TempRegisters[ro]&=~(0x10);
			}
			else if(reg==REG_MISC1)
			{
				ADS_TempRegisters[ro]&=~(0xdf);
			}
			else if(reg==REG_MISC2)
			{
				ADS_TempRegisters[ro]&=~(0xff);
			}
			else if(reg==REG_CONFIG4)
			{
				ADS_TempRegisters[ro]&=~(0xf5);
			}

			if(ADS_Shadow[ro]!=ADS_TempRegisters[ro])
			{
				WriteRegister(reg,ADS_TempRegisters[ro],1<<module);
				ADS_Shadow[ro]=ADS_TempRegisters[ro];
			}
		}
	}
	updateRegisterFlag=false;
}


void EEG_CopyShadowToTemp()
{
	memcpy(ADS_TempRegisters,ADS_Shadow,REGISTERS*MODULES);
}


void ReadADSRegisters(void)
{
	int ro;	//	Register offset
	for(int module=0;module<MODULES;module++)
	{
		//	Set CS for the Module
		SetCS(1<<module);
		//	Set Register Read
		Spi(OP_RREG);
		//	Burst read the entire module register set
		Spi(REGISTERS-1);
			for(int reg=0;reg<REGISTERS;reg++)
			{	ro=module*REGISTERS+reg;
				ADS_Shadow[ro]=Spi(0);
				ADS_TempRegisters[ro]=ADS_Shadow[ro];
			}
			//	Clear CS
		SetCS(0);
	}

}

const char *REGISTER_STRINGS[]=
{
		"ID", "CONFIG1", "CONFIG2", "CONFIG3", "LOFF", "CH1SET", "CH2SET", "CH3SET", "CH4SET", "CH5SET", "CH6SET", "CH7SET","CH8SET",
		"BIAS_SENSP","BIAS_SENSN","LOFF_SENSP","LOFF_SENSN","LOFF_FLIP","LOFF_STATP","LOFF_STATN","GPIO","MISC1","MISC2","CONFIG4"
};
