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
 * comm_protocol.c
 *
 *  Created on: Nov 19, 2016
 *      Author: nick
 */
#include <stdlib.h>
#include "comm_protocol.h"
#include "../EEG-SPI/eeg.h"
#include "serial.h"


#define HEADER					('$')
#define TIMEOUT					(10)

typedef enum {ST_HEADER, ST_COMMAND, ST_LENGTH, ST_PAYLOAD, ST_CHECKSUM} STATE;

static STATE state;
static PACKET rcvPacket;

uint8_t GetChecksum(PACKET *packet);
void ParsePacket(PACKET *packet);
volatile uint16_t tmr_Serial;

int CreatePacket(COMMAND command, uint8_t *data, uint8_t length, PACKET *packet)
{
	if(length>MAX_PAYLOAD_LENGTH) return 1;

	packet->header=HEADER;
	packet->command=command;
	packet->length=length;
	for(int i=0;i<length;i++) packet->data[i]=data[i];
	packet->checksum=GetChecksum(packet);
	return 0;
}

void ProcessCommByte(uint8_t data)
{
	static uint8_t ptrPayload=0;
	extern PACKET *parse;


	if(tmr_Serial==0) state=ST_HEADER;
	tmr_Serial=TIMEOUT;

	switch(state)
	{
	case ST_HEADER:
		if(data==HEADER)
		{
			rcvPacket.header=data;
			ptrPayload=0;
			state=ST_COMMAND;
		}
		break;

	case ST_COMMAND:
		rcvPacket.command=data;
		state=ST_LENGTH;
		break;

	case ST_LENGTH:
		rcvPacket.length=data;
		if(data>MAX_PAYLOAD_LENGTH) state=ST_HEADER;
		if(data==0) state=ST_CHECKSUM;
		else
			{
				//pktData=(byte *)malloc(rcvPacket.length);
				//rcvPacket.data=pktData;
				state=ST_PAYLOAD;
			}
		break;

	case ST_PAYLOAD:
		rcvPacket.data[ptrPayload++]=data;
		if(ptrPayload==rcvPacket.length) state=ST_CHECKSUM;
		break;

	case ST_CHECKSUM:
		if(data==GetChecksum(&rcvPacket))
			{
				parse=&rcvPacket;
			}
		state=ST_HEADER;
		break;

	default:
		state=ST_HEADER;
		break;
	}
}

uint8_t GetChecksum(PACKET *packet)
{
	uint8_t checksum=0;
	checksum+=packet->header;
	checksum+=packet->command;
	checksum+=packet->length;
	for(int i=0;i<packet->length; i++) checksum+=packet->data[i];
	return checksum;
}

void ParsePacket(PACKET *packet)
{
	switch(packet->command)
	{
	case 0x10:	//	Update Registers
		//WriteADSRegisters(packet->data);
		CopyToTempRegisters(packet->data);
		PACKET pkt;
		CreatePacket(0x11, NULL, 0, &pkt);	//	Send ACK
		SendPacket(&pkt);
		break;

	case 0x20:	//	Read Registers
		ReadADSRegisters();
		CreatePacket(0x21, ADS_Shadow, MODULES*REGISTERS, &pkt);
		SendPacket(&pkt);
		break;

	}
	//	Finally, free data memory
	//free(packet->data);
}
