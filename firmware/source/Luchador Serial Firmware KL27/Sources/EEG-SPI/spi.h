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
 * spi.h
 *
 *  Created on: Sep 29, 2016
 *      Author: nick
 */

#ifndef SOURCES_EEG_SPI_SPI_H_
#define SOURCES_EEG_SPI_SPI_H_

#include <stdbool.h>
#include <stdint.h>

#define		OP_START	(0x08)
#define		OP_STOP		(0x0A)
#define		OP_RREG		(0x20)
#define 	OP_WREG		(0x40)
#define 	OP_RDATA	(0x12)
#define 	OP_SDATAC	(0x11)
#define 	OP_RESET	(0x06)

#define		CS0			(0x01)
#define 	CS1			(0x02)
#define 	CS2			(0x04)
#define		CS3			(0x08)

void Init_Spi(void);
uint8_t ReadRegister(uint8_t reg, uint8_t cs);
void WriteRegister(uint8_t reg, uint8_t data, uint8_t cs);
void StopRDCMode(void);
bool GetRegisterMode(void);
void StartConversions(void);
void StopConversions(void);
void ReadEEG(void);
void SetCS(uint8_t cs);
uint8_t Spi(uint8_t sdata);
void Spi_SendReset(int cs);
#endif /* SOURCES_EEG_SPI_SPI_H_ */
