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
 * serial.c
 *
 *  Created on: Sep 30, 2016
 *      Author: nick
 */


/*
 * The current iteration of the serial module uses DMA from the PEX lpuart component for the data port (uart0) and
 * interrupt mode for uart1.  DMA was necessary because the high baud rate (921600) of uart0 was keeping the CPU in the
 * ISR for 90% of the time and eventually it would miss a character.  DMA isn't needed for the slower (115200) baud rate of
 * uart1.  Since the DMA implementation on receive doesn't allow a circular buffer, it is necessary to restart the DMA
 * receive transfer either when a command is received or the buffer is filled.  This makes it necessary for the host to wait
 * for the CLI to parse the command and respond before issuing a new command otherwise uart will not be receiving data into
 * the DMA buffer and will miss commands or characters.  There is no way to cleanly restart DMA receive in the middle of a
 * reception without running the risk of missing data.
 * Nick 12-10-18
 */

#include "EEG-SPI/spi.h"
#include "LEDS/leds.h"
#include "EEG-SPI/eeg.h"
#include "utils.h"
#include "comm_protocol.h"
#include "serial.h"
#include "cli.h"
#include "settings.h"
#include "Queue.h"
#include "MKL27Z4.h"
#include "Cpu.h"
#include "Events.h"
#include "AuxGpio.h"
#include "DataUart.h"
#include "MarkUart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef WIFI

#define RXBUFSIZE	(128)

char stringbuf1[2048];
static int mark;
static uint8_t dataUartRxBuf[RXBUFSIZE];
static uint8_t markUartRxBuf[RXBUFSIZE];

static void CheckParseDataUart();
static void CheckParseMarkUart();



void SendPacket(PACKET *packet);
void EnqueMemory(uint8_t *data);
static void CreateDataString(int ptr, uint32_t pCounter, char *data);



void Init_Serial(void)
{
	LPUART_DRV_DmaReceiveData(DataUart_IDX, dataUartRxBuf, RXBUFSIZE);
	LPUART_DRV_ReceiveData(MarkUart_IDX, markUartRxBuf, RXBUFSIZE);

}



void Serial_Close()
{
	LPUART_DRV_Deinit(DataUart_IDX);
	LPUART_DRV_Deinit(MarkUart_IDX);
}

static void CheckParseDataUart()
{

	static uint8_t input[RXBUFSIZE];
	//	Check how many bytes have been received
	uint32_t br;
	LPUART_DRV_DmaGetReceiveStatus(DataUart_IDX, &br);
	//	If the buffer is full and reception stopped, restart reception and exit
	if(br==0)
	{
		LPUART_DRV_DmaReceiveData(DataUart_IDX,dataUartRxBuf,RXBUFSIZE);
		return;
	}
	int btr=RXBUFSIZE-br;
	for(int i=0;i<btr;i++)
	{
		if(SET_ECHO_DATA)
		{
			while(LPUART_DRV_DmaSendData(DataUart_IDX, &dataUartRxBuf[i],1)!=0);
		}
		if(dataUartRxBuf[i]=='\r')
		{
			//	Copy rxbuffer to parse line without the '\r'
			memcpy(input, dataUartRxBuf,i);
			//	Now start a new reception
			LPUART_DRV_DmaAbortReceivingData(DataUart_IDX);
			LPUART_DRV_DmaReceiveData(DataUart_IDX,dataUartRxBuf,RXBUFSIZE);
			//	Null terminate the string
			input[i]=0;
			//	Parse and get the response
			Cli_Parse((char *)input,stringbuf1, "CLI:");
			strcat(stringbuf1, "\n\r");
			//	Send the response
			Serial_SendString(stringbuf1,PORT_DATA);
			return;
		}

	}
}

static void CheckParseMarkUart()
{
	static int si=0;
	int i=0;
	static uint8_t input[RXBUFSIZE];
	//	Check how many bytes have been received
	uint32_t br;
	LPUART_DRV_GetReceiveStatus(MarkUart_IDX, &br);
	//	If the buffer is full and reception stopped, restart reception and exit
	if(br==0)
	{
		LPUART_DRV_ReceiveData(MarkUart_IDX,dataUartRxBuf,RXBUFSIZE);
		si=0;
		return;
	}
	int btr=RXBUFSIZE-br-si;
	for(i=si;i<btr+si;i++)
	{
		//	Echo received character
		if(SET_ECHO_MARK)
		{
			while(LPUART_DRV_SendData(MarkUart_IDX, &markUartRxBuf[i],1)!=0);
		}
		//	If return received, parse the buffer
		if(markUartRxBuf[i]=='\r')
		{
			//	Copy rxbuffer to parse line without the '\r'
			memcpy(input, markUartRxBuf,i+si);
			//	Now start a new reception
			LPUART_DRV_AbortReceivingData(MarkUart_IDX);
			LPUART_DRV_ReceiveData(MarkUart_IDX,markUartRxBuf,RXBUFSIZE);
			//	Null terminate the string
			input[i]=0;
			//	Parse and get the response
			Cli_Parse((char *)input,stringbuf1, "\n\rCLI:");
			strcat(stringbuf1,"\n\r-->");
			//	Send the response
			Serial_SendString(stringbuf1,PORT_MARK);
			si=0;
			return;
		}

	}
	si=i;
}

void Serial_Process(void)
{
	static uint32_t pCounter=0;
	static char cdata[512];


	//	Check for new data
	if(cPtr!=pPtr)
	{
		CreateDataString(cPtr,pCounter++, cdata);
		Serial_SendString(cdata, PORT_DATA);

		if(++cPtr==SAMPSIZE)
		{
			cPtr=0;
		}
	}

	CheckParseDataUart();
	CheckParseMarkUart();



}


static void CreateDataString(int ptr, uint32_t pCounter, char *data)
{
	char buf[100];
	//	Add counter value to string
	sprintf(data, "DATA:%lu,", pCounter);
	//	Add channel data
	for(int x=0;x<32;x++)
	{
		sprintf(buf,"%ld,", eegData[ptr][x]);
		strcat(data,buf);
	}
	//	Add Mark
	sprintf(buf,"%d\n\r",mark);	//	<--	Add mark when available
	if(mark!=0) mark=0;
	strcat(data,buf);
}

void Serial_SetMark(int m)
{
	mark=m;
}

int Serial_GetMark()
{
	return mark;
}

void Serial_SendString(char * string, int port)
{
	switch(port)
	{
	case PORT_DATA:
		while(LPUART_DRV_DmaSendData(DataUart_IDX,(uint8_t *)string, strlen(string))!=0);
		break;

	case PORT_MARK:
		while(LPUART_DRV_SendData(MarkUart_IDX,(uint8_t *)string, strlen(string))!=0);
		break;


	}
}

#endif


