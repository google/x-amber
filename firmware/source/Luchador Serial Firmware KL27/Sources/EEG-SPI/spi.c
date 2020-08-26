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
 * spi.c
 *
 *  Created on: Sep 29, 2016
 *      Author: nick
 */

#include "spi.h"
#include "eeg.h"
#include "MKL27Z4.h"
#include "spiCom1.h"
#include "GpioEeg.h"
#include <stdint.h>

#define SPI_LOW_LEVEL

volatile bool spiRdy;
static bool registerMode=0;
bool converting=0;
bool eegDataRdy=0;

extern int smpPtr;




//	The original PEX methods were extremely slow.  Low level SPI transfer is about 4x faster
uint8_t Spi(uint8_t sdata)
{
#ifdef SPI_LOW_LEVEL
	uint8_t rdata;

	//AUX1_SetVal(AUX1_DeviceData);
	//	Wait for transmit buffer ready
	while((SPI0_S&SPI_S_SPTEF_MASK)==0);
	//	Clear the receive buffer
	while((SPI0_S & (SPI_S_SPRF_MASK))) rdata=SPI0_DL;
	//	Write to the SPI register
	SPI0_DL=sdata;
	//	Wait for data to complete
	while (!(SPI0_S & (SPI_S_SPRF_MASK)));
	//	Return the data
	//AUX1_ClrVal(AUX1_DeviceData);
	return SPI0_DL;
#else
	LDD_TError error;
	uint8 rdata;
	spiRdy=0;
	AUX1_SetVal(AUX1_DeviceData);
	error=SM1_ReceiveBlock(spiPtr,&rdata,1);
	error=SM1_SendBlock(spiPtr, &sdata,1);
	while(!spiRdy);
	AUX1_ClrVal(AUX1_DeviceData);
	return rdata;
#endif
}

uint8_t ReadRegister(uint8_t reg, uint8_t cs)
{
	uint8_t rdata;

	//if(!registerMode) EnterRegisterMode();
	SetCS(cs);
	Spi(OP_RREG+reg);
	Spi(0);
	rdata=Spi(0);
	SetCS(0);
	return rdata;
}

void WriteRegister(uint8_t reg, uint8_t data, uint8_t cs)
{
	//if(!registerMode) EnterRegisterMode();
	SetCS(cs);
	Spi(OP_WREG+reg);
	Spi(0);
	Spi(data);
	SetCS(0);
}

void SetCS(uint8_t cs)
{
	if(cs==0)
	{
		GPIO_DRV_SetPinOutput(GPIO_CS0);
		GPIO_DRV_SetPinOutput(GPIO_CS1);
		GPIO_DRV_SetPinOutput(GPIO_CS2);
		GPIO_DRV_SetPinOutput(GPIO_CS3);
		return;
	}
	if(cs & CS0) GPIO_DRV_ClearPinOutput(GPIO_CS0);
	if(cs & CS1) GPIO_DRV_ClearPinOutput(GPIO_CS1);
	if(cs & CS2) GPIO_DRV_ClearPinOutput(GPIO_CS2);
	if(cs & CS3) GPIO_DRV_ClearPinOutput(GPIO_CS3);

}

void StopRDCMode(void)
{
	SetCS(CS0+CS1+CS2+CS3);
	Spi(OP_SDATAC);
	SetCS(0);
	registerMode=1;
}

void Spi_SendReset(int cs)
{
	SetCS(cs);
	Spi(OP_RESET);
	SetCS(0);

}

bool GetRegisterMode(void)
{
	return registerMode;
}


void StartConversions(void)
{
	SetCS(CS0+CS1+CS2+CS3);
	//SetCS(CS0);
	Spi(OP_START);
	SetCS(0);
	converting=1;
}


void StopConversions(void)
{
	SetCS(CS0+CS1+CS2+CS3);
	Spi(OP_STOP);
	SetCS(0);
	converting=0;
}

void ReadEEG(void)
{
	uint32_t data;
	int i=0;
	int ch=0;
	extern bool updateRegisterFlag;

	//	Do any register writes first
	if(updateRegisterFlag)
	{
		//AUX1_SetVal(AUX1_DeviceData);
		WriteChangedRegisters();
		//AUX1_ClrVal(AUX1_DeviceData);
	}
	//	Now read the EEG data
	for(;i<4;i++)
	{
		//	Select CS Line
		switch(i)
		{
		case 0:
			SetCS(CS0);
			break;

		case 1:
			SetCS(CS1);
			break;

		case 2:
			SetCS(CS2);
			break;

		case 3:
			SetCS(CS3);
			break;
		}

		Spi(OP_RDATA);
		data=(uint32_t)Spi(0)<<16;
		eegStatus[pPtr][i]=data;
		data=(uint32_t)Spi(0)<<8;
		eegStatus[pPtr][i]+=data;
		data=(uint32_t)Spi(0);
		eegStatus[pPtr][i]+=data;

		for(ch=0;ch<8;ch++)
		{
			data=(uint32_t)Spi(0)<<24;
			eegData[pPtr][i*8+ch]=data;
			data=(uint32_t)Spi(0)<<16;
			eegData[pPtr][i*8+ch]+=data;
			data=(uint32_t)Spi(0)<<8;
			eegData[pPtr][i*8+ch]+=data;
			eegData[pPtr][i*8+ch]=eegData[pPtr][i*8+ch]>>8;	//	Sign/data correct
		}
		SetCS(0);
	}
	pPtr++;
	if(pPtr==cPtr)	//	CRASH!
	{
		//PE_DEBUG();
		cPtr=0;
	}
	if(pPtr==SAMPSIZE)
	{
		//eegDataRdy=1;
		pPtr=0;
	}

}

int EEG_GetCurrentChannelValue(int channel)
{
	int p=pPtr;

	p--;
	if(p<0) p=SAMPSIZE;
	return eegData[p][channel-1];
}


