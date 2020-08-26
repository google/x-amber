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
 * eeg.h
 *
 *  Created on: Sep 29, 2016
 *      Author: nick
 */

#ifndef SOURCES_EEG_SPI_EEG_H_
#define SOURCES_EEG_SPI_EEG_H_

#include <stdbool.h>
#include <stdint.h>

//	ADS1299 REGISTERS
#define 	REG_ID			(0x00)
#define		REG_CONFIG1		(0x01)
#define		REG_CONFIG2		(0x02)
#define		REG_CONFIG3		(0x03)
#define 	REG_LOFF		(0x04)
#define		REG_CH1SET		(0x05)
#define		REG_CH2SET		(0x06)
#define		REG_CH3SET		(0x07)
#define		REG_CH4SET		(0x08)
#define		REG_CH5SET		(0x09)
#define		REG_CH6SET		(0x0A)
#define		REG_CH7SET		(0x0B)
#define		REG_CH8SET		(0x0C)
#define		REG_BIAS_SENSP	(0x0D)
#define		REG_BIAS_SENSN	(0x0E)
#define		REG_LOFF_SENSP	(0x0F)
#define		REG_LOFF_SENSN	(0x10)
#define		REG_LOFF_FLIP	(0x11)
#define		REG_LOFF_STATP	(0x12)
#define		REG_LOFF_STATN	(0x13)
#define		REG_GPIO		(0x14)
#define		REG_MISC1		(0x15)
#define		REG_MISC2		(0x16)
#define		REG_CONFIG4		(0x17)

//	Register bits

#define		ID_REV_ID3		(0x80)
#define		ID_REV_ID2		(0x40)
#define		ID_REV_ID1		(0x20)
#define		ID_DEV_ID2		(0x08)
#define		ID_DEV_ID1		(0x04)
#define		ID_NU_CH2		(0x02)
#define		ID_NU_CH1		(0x01)

#define		CF1_DE			(0x40)
#define		CF1_CLK_EN		(0x20)
#define		CF1_DR2			(0x04)
#define		CF1_DR1			(0x02)
#define		CF1_DR0			(0x01)

#define		CF2_INT_CAL		(0x10)
#define		CF2_CAL_AMP0	(0x04)
#define		CF2_CAL_FREQ1	(0x02)
#define		CF2_CAL_FREQ0	(0x01)

#define		CF3_PD_REFBUF	(0x80)
#define		CF3_BIAS_MEAS	(0x10)
#define		CF3_BIASREF_INT	(0x08)
#define		CF3_PD_BIAS		(0x04)
#define		CF3_BLS			(0x02)
#define		CF3_BIAS_STAT	(0x01)

#define		LOFF_COMP_TH2	(0x80)
#define		LOFF_COMP_TH1	(0x40)
#define		LOFF_COMP_TH0	(0x20)
#define		LOFF_ILOFF1		(0x08)
#define		LOFF_ILOFF0		(0x04)
#define		LOFF_FLOFF1		(0x02)
#define		LOFF_FLOFF0		(0x01)

#define		CH_PD			(0x80)
#define		CH_GAIN2		(0x40)
#define		CH_GAIN1		(0x20)
#define		CH_GAIN0		(0x10)
#define		CH_SRB2			(0x08)
#define		CH_MUX2			(0x04)
#define		CH_MUX1			(0x02)
#define		CH_MUX0			(0x01)

#define		BIAS8			(0x80)
#define		BIAS7			(0x40)
#define		BIAS6			(0x20)
#define		BIAS5			(0x10)
#define		BIAS4			(0x08)
#define		BIAS3			(0x04)
#define		BIAS2			(0x02)
#define		BIAS1			(0x01)

#define		LOFF8			(0x80)
#define		LOFF7			(0x40)
#define		LOFF6			(0x20)
#define		LOFF5			(0x10)
#define		LOFF4			(0x08)
#define		LOFF3			(0x04)
#define		LOFF2			(0x02)
#define		LOFF1			(0x01)

#define		LFLIP8			(0x80)
#define		LFLIP7			(0x40)
#define		LFLIP6			(0x20)
#define		LFLIP5			(0x10)
#define		LFLIP4			(0x08)
#define		LFLIP3			(0x04)
#define		LFLIP2			(0x02)
#define		LFLIP1			(0x01)

#define		MISC_SRB1		(0x20)

#define		CF4_SINGLE		(0x08)
#define		CF4_PD_LOFF		(0x02)

#define SAMPSIZE			(100)
#define SENDBUFFER			(30)

#define MODULES			(4)
#define REGISTERS		(0x18)

extern volatile int32_t eegData[SAMPSIZE][32];
extern volatile uint32_t eegStatus[SAMPSIZE][4];
extern uint8_t ADS_Shadow[MODULES*REGISTERS];
extern const char *REGISTER_STRINGS[];
extern uint8_t ADS_TempRegisters[MODULES*REGISTERS];
extern bool updateRegisterFlag;

volatile int pPtr;
volatile int cPtr;

void Init_EEG(void);
void Reset_EEG(void);
void Do_EEG(void);
void ReportEEGRegisters(void);
void SimulateEEG(void);
void CopyToTempRegisters(uint8_t *data);
void ReadADSRegisters(void);
void WriteADSRegistersFast(void);
void WriteChangedRegisters(void);
void EEG_CopyShadowToTemp();
int EEG_GetCurrentChannelValue(int channel);


#endif /* SOURCES_EEG_SPI_EEG_H_ */
