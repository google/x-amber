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
 * comm_protocol.h
 *
 *  Created on: Nov 19, 2016
 *      Author: nick
 */

#ifndef SOURCES_SERIAL_COMM_PROTOCOL_H_
#define SOURCES_SERIAL_COMM_PROTOCOL_H_

#include <stdint.h>

#define MAX_PAYLOAD_LENGTH		(200)

typedef enum {CMD_EEGDATA} COMMAND;
typedef struct
{
	uint8_t header;
	uint8_t command;
	uint8_t length;
	uint8_t data[MAX_PAYLOAD_LENGTH];
	uint8_t checksum;
} PACKET;

int CreatePacket(COMMAND command, uint8_t *data, uint8_t length, PACKET *pktStruct);
void ProcessCommByte(uint8_t data);
void ParsePacket(PACKET *packet);

#endif /* SOURCES_SERIAL_COMM_PROTOCOL_H_ */
