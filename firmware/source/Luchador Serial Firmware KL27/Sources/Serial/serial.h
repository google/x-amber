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
 * serial.h
 *
 *  Created on: Sep 30, 2016
 *      Author: nick
 */

#ifndef SOURCES_SERIAL_SERIAL_H_
#define SOURCES_SERIAL_SERIAL_H_
#include "comm_protocol.h"
#include <stdbool.h>
#include <stdint.h>

#define BUFSIZE		128
#define QUESIZE		10

#define PORT_DATA	0
#define PORT_MARK	1


typedef struct
{
	char data[QUESIZE][BUFSIZE];
	int ptr;
	int producer, consumer;
	int length;
} Buf_T;


extern char sbuf[256];
extern Buf_T markInBuf, dataInBuf;

void Init_Serial(void);
void Serial_Test(void);
void Serial_Process(void);
void Parse_Serial(void);
void Serial_SendString(char * string, int port);
void DequeMemory(void);
void SendPacket(PACKET *packet);
void Serial_SetRegister(int reg, int value);
void Serial_ProcessChar(Buf_T *buffer, char c);
void Serial_SetMark(int m);
int Serial_GetMark();
void Serial_Close();
void Serial_StartReceiveDataPort();
void Serial_PushDataPortCharacter();
void Serial_StartReceiveMarkPort();
void Serial_PushMarkPortCharacter();
void Serial_SendChar(uint8_t *c, int port);
void Serial_DmaSendUart0(uint8_t *data, int length);

#endif /* SOURCES_SERIAL_SERIAL_H_ */
