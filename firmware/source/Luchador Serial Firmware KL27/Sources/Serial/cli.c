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
 * cli.c
 *
 *  Created on: Jul 12, 2018
 *      Author: nick
 */
#include "cli.h"
#include "serial.h"
#include "EEG-SPI/eeg.h"
#include "EEG-SPI/spi.h"
#include "settings.h"
#include "version.h"
#include "Cpu.h"
#include "utils.h"
#include "Adc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <MKL27Z4.h>

static void StripChars(char* str, char c) ;
static void Chon(int channel, bool state);
static int GetChannelGain(int channel);
static double GetChannelmV(int channel);
static void FixString(char *str);
static float Adc2V(uint16_t adcVal);
static float GetVsys();
static float Get3V3();
static float Get2V5P();
static float Get2V5N();

bool Cli_Parse(char * input, char * output, char * prefix)
{
	char buf[32];
	//	Check if length<128
	if(strlen(input)>128)
	{
		sprintf(output, "%sERROR: Input exceeds 128 bytes",prefix);
		return false;
	}

	//strip control characters
	StripChars(input, '\n');
	StripChars(input, '\r');

	//	Parse deletes
	FixString(input);

	//	Convert to lowercase
	for(int x=0;(int)input[x];x++) input[x]=tolower(input[x]);

	//	Parse input into [command][arguments]

	char command[20]="";
	char args[20]="";
	char arg1[10]="";
	char arg2[10]="";

	//	Clear command and args
	SplitString(input, command,' ',0,20);
	SplitString(input, args, ' ',1,20);
	//char *command=strtok(input, " ");
	//char *args=strtok(NULL, " ");

	if(strcmp(command,"sr")==0)		//	<--Set register
	{
		SplitString(args, arg1, ',',0,10);
		SplitString(args, arg2, ',',1,10);
		char *reg=arg1;
		char *value=arg2;
		int ireg=(int)strtol(reg,NULL,16);
		int ival=(int)strtol(value,NULL,16);

		if(ireg>95 || ireg<0)
		{
			sprintf(output, "%sERROR: SR: Register out of bounds",prefix);
			return false;
		}
		if(ival<0 || ival>255)
		{
			sprintf(output, "%sERROR: SR: Register value out of bounds",prefix);
			return false;
		}
		Serial_SetRegister(ireg,ival);
		sprintf(output, "%sOK: Register %x set to %x", prefix,ireg, ival);
		return true;
	}
	else if(strcmp(command, "rr")==0)	//	<--Read register set
	{

		ReadADSRegisters();

		if(strlen(args)!=0)
		{
			int ireg=(int)strtol(args,NULL,16);
			sprintf(output,"\t%02x:%s:%02x\n\r",ireg,REGISTER_STRINGS[ireg%REGISTERS],ADS_Shadow[ireg]);
			return true;
		}

		output[0]=0;


		int i=0;
		for(int m=0;m<MODULES;m++)
		{
			sprintf(buf, "\n\rModule %d (CH %d-%d)\n\r",m+1,(m*8)+1, (m*8)+8);
			strcat(output,buf);
			for(int r=0;r<REGISTERS;r++)
			{
				sprintf(buf,"\t%02x:%s:%02x\n\r",i,REGISTER_STRINGS[r],ADS_Shadow[i]);
				strcat(output,buf);
				i++;
			}
		}
		return true;
	}
	else if(strcmp(command, "rq")==0)	//	<--	Read registers quick
	{
		ReadADSRegisters();
		sprintf(output,"%s",prefix);
		for(int r=0;r<(REGISTERS*MODULES);r++)
		{
			sprintf(buf, "%02x,", ADS_Shadow[r]);
			strcat(output, buf);
		}
		//	Remove last comma
		output[strlen(output)-1]=0;
		return true;
	}
	else if(strcmp(command, "rc")==0)	//	<--	Read channel value
	{
		if(strlen(args)==0)
		{
			sprintf(output, "%sERROR:RC: No channel specified",prefix);
			return false;
		}
		int ival=(int)strtol(args, NULL, 16);
		if(ival>32 || ival<1)
		{
			sprintf(output, "%sERROR:RC: Channel out of range",prefix);
			return false;
		}
		sprintf(output, "%s%d",prefix, EEG_GetCurrentChannelValue(ival));
		return true;

	}
	else if(strcmp(command, "sg")==0 || strcmp(command, "schg")==0)	//	<--	Set gain
	{
		//	Extract channel and gain values
		SplitString(args, arg1,',',0,10);
		SplitString(args,arg2,',',1,10);
		int ch=atoi(arg1);
		int g=atoi(arg2);
		if(ch>32 || ch<1)
		{
			sprintf(output, "%sERROR:SG: Channel out of bounds",prefix);
			return false;
		}
		//	Calculate module and offset
		int mod=((ch-1)/8)*REGISTERS;
		int offset=((ch-1)%8)+0x05;
		int val=0;
		switch(g)
		{
			case 1:
				val=0x00;
				break;

			case 2:
				val=0b001;
				break;

			case 4:
				val=0b010;
				break;

			case 6:
				val=0b011;
				break;

			case 8:
				val=0b100;
				break;

			case 12:
				val=0b101;
				break;

			case 24:
				val=0b110;
				break;

			default:
				sprintf(output, "%sERROR:SG: Invalid gain value",prefix);
				return false;
				break;

		}

		ADS_TempRegisters[mod+offset]&=(uint8_t)(~(0b111<<4));		//	Clear the gain settings
		ADS_TempRegisters[mod+offset]|=(uint8_t)(val<<4);			//	Set the gain settings
		updateRegisterFlag=true;
		sprintf(output, "%sCH %d gain set to %d",prefix, ch, g);
		return true;
	}
	else if(strcmp(command, "mark")==0)		//	<--	Set the mark value
	{
		if(strlen(args)==0)
		{
			sprintf(output,"%sMARK=%d",prefix,Serial_GetMark());
			return true;
		}
		int mark=atoi(args);
		Serial_SetMark(mark);
		sprintf(output, "%sMark set to [%d]",prefix,mark);
		return true;
	}
	else if(strcmp(command, "chon")==0)
	{
		SplitString(args,arg1,',',0,10);
		SplitString(args, arg2, ',',1,10);
		char *ch=arg1;
		bool state=atoi(arg2);
		if(strcmp(ch, "all")==0)
		{
			for(int i=1;i<33;i++) Chon(i,state);
			sprintf(output, "%sAll channels set to %s",prefix, state==true?"ON":"OFF");
			return true;
		}
		int chan=atoi(ch);
		if(chan>32 || chan<1)
		{
			sprintf(output, "%sERROR:CHON: Channel out of range",prefix);
			return false;
		}
		Chon(chan,state);
		sprintf(output, "%sChannel [%d] turned [%s]",prefix,chan, state==true?"ON":"OFF");
		return true;
	}
	else if(strcmp(command, "rv")==0)	//	<--	Read channel voltage in mv
	{
		int ch=atoi(args);
		if(ch>32 || ch<1)
		{
			sprintf(output, "%sERROR:RV: Channel out of range",prefix);
			return false;
		}
		double mv=GetChannelmV(ch);
		sprintf(output, "%sCH %d=%f mv",prefix, ch, mv);
		return true;
	}
	else if(strcmp(command, "test")==0)	//	<--	Setup device for test signals
	{
		ADS_TempRegisters[0x02]=0xd0;
		ADS_TempRegisters[0x1a]=0xd0;
		ADS_TempRegisters[0x32]=0xd0;
		ADS_TempRegisters[0x4a]=0xd0;

		for(int mod=0;mod<MODULES;mod++)
		{
			for(int r=0;r<8;r++)
			{
				ADS_TempRegisters[mod*REGISTERS+r+0x05]=0x05;
			}
		}
		updateRegisterFlag=true;
		sprintf(output, "%sTest mode turned on",prefix);
		return true;

	}
	else if(strcmp(command, "reset")==0)
	{
		Reset_EEG();
		StartConversions();
		ReadADSRegisters();
		//EEG_CopyShadowToTemp();
		sprintf(output, "%sEEG Reset",prefix);
		return true;
	}

	else if(strcmp(command, "bootloader")==0)
	{
		//	Stop IRQ
		__disable_irq();
		//	Close serial ports
		Serial_Close();
		// Variables
		uint32_t runBootloaderAddress;
		void (*runBootloader)(void * arg);
		runBootloaderAddress = **(uint32_t **)(0x1c00001c);
		runBootloader = (void (*)(void * arg))runBootloaderAddress;
		// Start the bootloader.
		runBootloader(NULL);
		while(1);
	}
	else if(strcmp(command,"ver")==0)
	{
		char fwversion[20];
		char hwversion[20];
		Version_GetVersionString(fwversion);
		Version_GetHwVersionString(hwversion);
		sprintf(output, "%sFW VERSION:%s, HW VERSION:%s\n\r",prefix,fwversion,hwversion);
		return true;
	}
	else if(strcmp(command, "ser")==0)
	{
		char serial[32];

		Version_GetSerialNumber(serial);
		sprintf(output, "%sSerial:%s",prefix,serial);
		return true;
	}
	else if(strcmp(command, "setser")==0)
	{
		if(Version_SetSerialNumber(args))
		{
			sprintf(output, "%sSerial number set to:%s",prefix,args);
			return true;
		}
		else
		{
			sprintf(output, "%sERROR:Could not set serial number",prefix);
			return true;
		}
	}
	else if(strcmp(command, "diag")==0)
	{
		sprintf(output, "%s+VSYS=%f, +3.3V=%f, +2.5V=%f, -2.5V=%f",prefix, GetVsys(), Get3V3(), Get2V5P(), Get2V5N());
		return true;
	}
	else if(strcmp(command, "help")==0)
	{
		sprintf(output, "%sCOMMAND SET\n\r",prefix);
		strcat(output, "sr [reg],[value]\t\tSet register to value.\n\r");
		strcat(output, "rr [reg]\t\t\tRead register.  With [reg] specified, will read entire register set.\n\r");
		strcat(output, "rq\t\t\t\tRead entire register set quickly.\n\r");
		strcat(output, "rc [chan]\t\t\tRead channel ADC value.\n\r");
		strcat(output, "sg [chan],[gain]\t\tSet channel gain.\n\r");
		strcat(output, "mark [value]\t\t\tSet mark to value.\n\r");
		strcat(output, "chon [chan],[state]\t\tTurn channel on/off.  Replace [chan] with [all] to set all channels.\n\r");
		strcat(output, "rv [chan]\t\t\tRead channel voltage.\n\r");
		strcat(output, "test\t\t\t\tSet registers to measure test signals.\n\r");
		strcat(output, "reset\t\t\t\tReset the ADS1299s.\n\r");
		strcat(output, "poff\t\t\t\tTurn off the system power.\n\r");
		strcat(output, "'$'\t\t\t\tToggle echo.\n\r");
		return true;
	}
	else
	{
		uint8_t foo=command[0];
		if(foo==0xff)
		{
			output[0]=0;
			return false;
		}
		sprintf(output, "%sERROR: Unknown command [%s]",prefix, command);
		return false;
	}

	return true;

}

static void Chon(int channel, bool state)
{
	int mod=((channel-1)/8)*REGISTERS;
	int offset=((channel-1)%8)+0x05;
	int chreg=mod+offset;
	int miscreg=mod+0x15;
	int biaspreg=mod+0x0d;
	int biasnreg=mod+0x0e;
	int chbit=1<<((channel-1)%8);

	//	Set the first bias config register
	ADS_TempRegisters[0*REGISTERS+0x03]=0b11101100;		//	PDREFBUF, BIASREF_INT, PD_BIAS
	//	Set the other registers
	ADS_TempRegisters[1*REGISTERS+0x03]=0b01100000;		//	BIAS stuff off
	ADS_TempRegisters[2*REGISTERS+0x03]=0b01100000;
	ADS_TempRegisters[3*REGISTERS+0x03]=0b01100000;

	if(state)
	{
		ADS_TempRegisters[chreg]&=~0x08;			//	DisConnect SRB2
		ADS_TempRegisters[chreg]&=(0b01111000);		//	Connect channel to normal electrode config and turn on channel
		//	Enable bias bits
		ADS_TempRegisters[biaspreg]|=chbit;
		ADS_TempRegisters[biasnreg]|=chbit;
		//	Make sure SRB1 is connected
		ADS_TempRegisters[miscreg]=0x20;

	}
	else
	{
		//	Disable the channel
		ADS_TempRegisters[chreg]|=0x80;
		//	Disconnect SRB2
		//ADS_TempRegisters[chreg]&=~(0x08);
		//	Short inputs (As per TI datasheet)
		ADS_TempRegisters[chreg]|=0x01;
		//	Disable bias bits
		ADS_TempRegisters[biaspreg]&=~chbit;
		ADS_TempRegisters[biasnreg]&=~chbit;

	}
	updateRegisterFlag=true;
}

static int GetChannelGain(int channel)
{
	int mod=((channel-1)/8)*REGISTERS;
	int offset=((channel-1)%8)+0x05;
	int chbit=(ADS_TempRegisters[mod+offset]>>4)&0x07;
	switch(chbit)
	{
		case 0:
			return 1;

		case 1:
			return 2;

		case 2:
			return 4;

		case 3:
			return 6;

		case 4:
			return 8;

		case 5:
			return 12;

		case 6:
			return 24;

		default:
			return 0;
	}
}

static double GetChannelmV(int channel)
{
	int rc=EEG_GetCurrentChannelValue(channel);
	int gain=GetChannelGain(channel);

	return 1000.0*(double)rc*((9.0/(double)gain)/pow(2,24));

}

//	Removes the character from the string
static void StripChars(char* str, char c)
{
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

//	Remove 'backspace' character from string
static void FixString(char *str)
{
	int ni=0;
	int oi=0;
	while(str[ni])
	{
		if(str[ni]==0x8)
		{
			if(oi!=0) oi--;
			ni++;
		}
		else str[oi++]=str[ni++];


	}
	str[oi]=0;
}

//	Converts an ADC value to a voltage
static float Adc2V(uint16_t adcVal)
{
	const float REF=2.5f;
	const float FS=65535.0f;

	return REF*(float)adcVal/FS;
}

static float GetVsys()
{
	const float CV=4.3887f;	//	Conversion value

	//	Set mux2 to b channel
	ADC0_CFG2|=ADC_CFG2_MUXSEL_MASK;
	ADC16_DRV_ConfigConvChn(Adc_IDX,0,&Adc_VSYS);
	ADC16_DRV_WaitConvDone(Adc_IDX,0);
	float v=Adc2V(ADC16_DRV_GetConvValueRAW(Adc_IDX,0));
	return v*CV;
}

static float Get3V3()
{
	const float CV=2.0f;	//	Conversion value
	ADC16_DRV_ConfigConvChn(Adc_IDX,0,&Adc_3_3V);
	ADC16_DRV_WaitConvDone(Adc_IDX,0);
	float v=Adc2V(ADC16_DRV_GetConvValueRAW(Adc_IDX,0));
	return v*CV;
}

static float Get2V5P()
{
	const float CV=2.0f;	//	Conversion value
	ADC16_DRV_ConfigConvChn(Adc_IDX,0,&Adc_2_5VP);
	ADC16_DRV_WaitConvDone(Adc_IDX,0);
	float v=Adc2V(ADC16_DRV_GetConvValueRAW(Adc_IDX,0));
	return v*CV;
}

static float Get2V5N()
{
	const float R1=102.0f;
	const float R2=40.2f;
	const float VR=2.5f;

	//	Set mux2 to a channel
	ADC0_CFG2&=~ADC_CFG2_MUXSEL_MASK;
	//ADC0_CFG2|=ADC_CFG2_MUXSEL_MASK;
	ADC16_DRV_ConfigConvChn(Adc_IDX,0,&Adc_2_5VN);
	ADC16_DRV_WaitConvDone(Adc_IDX,0);
	float v=Adc2V(ADC16_DRV_GetConvValueRAW(Adc_IDX,0));
	float i=(VR-v)/R2;
	v=VR-(R1+R2)*i;
	return v;
}
