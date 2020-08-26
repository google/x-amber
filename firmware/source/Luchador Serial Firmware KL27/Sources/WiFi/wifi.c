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
 * wifi.c
 *
 *  Created on: Sep 28, 2016
 *      Author: nick
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "wifi.h"
#include "EEG-SPI/spi.h"
#include "LEDS/leds.h"
#include "EEG-SPI/eeg.h"
//#define DBG

#ifdef WIFI

typedef enum {WST_Init, WST_W_Init, WST_Connect, WST_W_Connect,WST_SetBaud, WST_StartServer, WST_W_StartServer, WST_Idle,
WST_Senddata1, WST_Senddata2, WST_Senddata3} WIFI_STATES;
typedef enum {WRS_Idle, WRS_Busy, WRS_OK, WRS_ERROR} WIFI_RESPONSE;

LDD_TDeviceData *WifiPtr;
char w_inbuf[WIFIBUFSIZE];
char w_outbuf[WIFIBUFSIZE];
char w_line[WIFIBUFSIZE];
int w_txPtr=0;
int w_rxPtr=0;
char w_inchar[5];
volatile uint16 tmr_wifi, tmr_wgp;
static WIFI_STATES w_State;
static WIFI_RESPONSE w_Response;
static bool w_Rdy=1;
static char buf[WIFIBUFSIZE];
static char response[50];
#ifdef DBG
static char debug[100][50];
static int dbgPtr=0;
#endif
static int connectionStatus;

static char command[2][256];
static int cmdPtr=0;
static int cmdRdy=0;
static int cmdBuf=0;

bool SendIntro=1;

static void SendWifiString(char *data, char *res);
static bool ConnectWifi(char *ssid, char *password);
static int WaitWifi(uint16 timeout);
static int CheckResponse(void);
static void SetMode(char *mode);
static void StartServer(char *mode, char *port);
static void SendString(char *string);
static void AddtoCommand(char *data);
static void Parse_Command(char *cmd);
static void SetCommand(char *var, char *val);
static void QueryCommand(char *var);
static void ExecuteCommand(char *var);
static bool SplitVal(char *val, char *val1, char *val2);

static char ebuf[2048]={0};
static char ibuf[10];
static int sCounter=0;
//extern uint8 Spi(uint8);
static void SetBaudFast(void);
static void SendEEGData(void);

void Do_Wifi(void)
{
	int foo;
	extern bool converting;
	extern bool eegDataRdy;
	LDD_TError error;

	//	First parse any commands
	if(cmdRdy!=0)
	{
		Parse_Command(command[cmdRdy-1]);
		cmdRdy=0;
		SendString("Ready\r\n");
	}
	switch(w_State)
	{
	case WST_Init:
		connectionStatus=0;
		tmr_wifi=5000;
		SendWifiString("AT\r\n", "OK\r");
		SendWifiString("AT+RST\r\n", "ready\r");
		//SendWifiString("AT+CIOBAUD=460800\r\n", "OK\r");
		//while(!w_Rdy);
		//SetBaudFast();
		SetMode("1");
		SendWifiString("AT+CIPMUX=1\r\n", "OK\r");
		w_State=WST_W_Init;
		break;

	case WST_W_Init:
		if(CheckResponse()==1) w_State=WST_Connect; else if(CheckResponse()>1) w_State=WST_Init;
		break;

	case WST_Connect:
		ConnectWifi("luchador", "luchador");
		tmr_wifi=20000;
		w_State=WST_W_Connect;
		break;

	case WST_W_Connect:
		foo=CheckResponse();
		if(foo==1)
			{
				connectionStatus=1;
				w_State=WST_StartServer;
			}
		else if(foo>1)
			{
				w_State=WST_Init;
			}

		break;

	case WST_StartServer:
		StartServer("1","3000");
		w_State=WST_W_StartServer;
		break;

	case WST_W_StartServer:
		foo=CheckResponse();
		if(foo==1)
			{
				//connectionStatus=1;
				w_State=WST_Idle;
			}
		else if(foo>1)
			{
				w_State=WST_Init;
			}

		break;

	case WST_Idle:
		if((cPtr!=pPtr) && (w_Rdy)) w_State=WST_Senddata1;
		break;

	case WST_Senddata1:	//	Send request
			strcpy(buf, "AT+CIPSEND=0,");
			sprintf(buf, "%s%i",buf,SENDBUFFER*57);
			strcat(buf,"\r\n");
			SendWifiString(buf, "OK\r");
			w_State=WST_Senddata2;
			break;

	case WST_Senddata2:
			if(w_Rdy)
			{
				if(cPtr!=pPtr)
				{
					ebuf[0]=0;
					for(int i=0;i<8;i++)
					{
						if(i!=7) sprintf(ibuf, "%06X,", eegData[cPtr][i]);else sprintf(ibuf, "%06X\r\n", eegData[cPtr][i]);
						strcat(ebuf,ibuf);
					}
					error=AS1_SendBlock(WifiPtr, ebuf,strlen(ebuf));
					if(error==ERR_BUSY) break;
					sCounter++;
					if(++cPtr==SAMPSIZE)
					{
						cPtr=0;
					}
				}
			}
			if(sCounter==SENDBUFFER)
			{
				strcpy(response, "SEND OK\r");
				w_Rdy=0;
				sCounter=0;
				w_State=WST_Idle;
			}
			break;

	default:
		w_State=WST_Init;
		break;
	}

	if(connectionStatus & 0x02)
		{
			if(SendIntro)
			{
				SendIntro=0;
				SendString("LUCHADOR\r\n");
				SendString("REV 0.1\r\n");
				SendString("Ready\r\n");
			}
		}
	if(converting && eegDataRdy)
	{
		SendEEGData();
		eegDataRdy=0;
	}

}


void Init_Wifi(void)
{
	WifiPtr=AS1_Init(NULL);
	LDD_TError error=AS1_ReceiveBlock(WifiPtr,w_inchar,1U);
}

void Parse_Wifi(void)
{
#ifdef DBG
	strcpy(debug[dbgPtr++], w_line);
	if(dbgPtr==100) dbgPtr=0;
#endif
	extern bool converting;

	//if(converting) StopConversions();
	if(!strcmp(w_line, response))
	{
		w_Rdy=1;
		w_Response=WRS_OK;
	}
	else if(!strcmp(w_line, "ERROR\r"))
	{
		w_Rdy=1;
		w_Response=WRS_ERROR;
	}
	else if(!strcmp(w_line, "WIFI DISCONNECT\r"))
	{
		connectionStatus=0;
	}
	else if(!strcmp(w_line, "WIFI CONNECTED\r"))
	{
		connectionStatus=1;
	}
	else if(strstr(w_line, ",CONNECT")!=NULL)
	{
		SendIntro=1;
		connectionStatus|=0x02;
	}
	else if(strstr(w_line, ",CLOSED")!=NULL)
	{
		connectionStatus&=~0x02;
		SendIntro=0;
	}
	else if(!strcmp(w_line, "Unlink\r"))
	{
		connectionStatus&=~0x02;
	}
	else if(strstr(w_line,"+IPD,0,")!=NULL)
	{
		AddtoCommand(w_line);
	}
	switch(w_State)
	{



	case WST_W_Connect:
		if(!strcmp(w_line, "FAIL\r"))
		{
			w_Rdy=1;
			w_Response=WRS_ERROR;
		}
		break;

	default:
		break;
	}

}

static void SendWifiString(char *data, char *res)
{
	LDD_TError error;

	while((!w_Rdy) && (tmr_wifi!=0));	//	Block until wifi responds with ok
	strcpy(response, res);
	do
	{
		error=AS1_SendBlock(WifiPtr, data,strlen(data));
	} while (error==ERR_BUSY);
	w_Rdy=0;
	w_Response=WRS_Busy;
}

static bool ConnectWifi(char *ssid, char *password)
{
	strcpy(buf, "AT+CWJAP=\"");
	strcat(buf, ssid);
	strcat(buf, "\",\"");
	strcat(buf, password);
	strcat(buf, "\"\r\n");
	SendWifiString(buf, "OK\r");

}

static void SetMode(char *mode)
{
	strcpy(buf, "AT+CWMODE=");
	strcat(buf, mode);
	strcat(buf, "\r\n");
	SendWifiString(buf, "OK\r");
}

static bool CheckWifiConnection()
{

}

static int WaitWifi(uint16 timeout)
{
	tmr_wifi=timeout;
	while((tmr_wifi!=0) && (w_Rdy==0));
	if(tmr_wifi==0) return 1;
	if(w_Response!=WRS_OK) return 2;
	return 0;
}

static int CheckResponse(void)
{
	if(w_Response==WRS_OK) return 1;
	if(tmr_wifi==0) return 2;
	if(w_Response==WRS_ERROR) return 3;
	return 0;
}

int GetConnectionStatus(void)
{
	return connectionStatus;
}

static void StartServer(char *mode, char *port)
{
	strcpy(buf, "AT+CIPSERVER=");
	strcat(buf, mode);
	strcat(buf, ",");
	strcat(buf, port);
	strcat(buf, "\r\n");
	SendWifiString(buf, "OK\r");
}

static void SendString(char *string)
{
	Set_Led(2,1);
	strcpy(buf, "AT+CIPSEND=0,");
	sprintf(buf, "%s%i",buf,strlen(string));
	strcat(buf,"\r\n");
	//strcat(buf,string);
	tmr_wifi=1000;
	SendWifiString(buf, "OK\r");
	SendWifiString(string, "SEND OK\r");
	while(!w_Rdy);
	Set_Led(2,0);
}

static void AddtoCommand(char *data)
{
	char channelId[10];
	char dataLength[10];
	int dLength=0;
	int state=0;
	int x=0;

	for(int i=0;data[i]!=0;i++)
	{
		switch(state)
		{
		case 0:	//	Find first comma
			if(data[i]==',') state++;
			break;

		case 1:	//	Get channel ID
			if(data[i]==',')
			{
				if(x<10) channelId[x++]=0; else state=10;
				x=0;
				state++;
				break;
			}
			channelId[x++]=data[i];
			if(x==5)
			{
				x=0;
				state++;

			}
			break;

		case 2://	Get data length
			if(data[i]==':')
			{
				if(x<10) dataLength[x++]=0;else state=10;
				dLength=atoi(dataLength);
				x=0;
				state++;
				break;
			}
			dataLength[x++]=data[i];
			if(x==5)
			{
				x=0;
				state++;
			}
			break;

		case 3://	copy data to command
			if(data[i]=='\n' || data[i]=='\r')
			{
				command[cmdBuf][cmdPtr++]=0;
				cmdPtr=0;
				cmdRdy=cmdBuf+1;
				if(cmdBuf==0) cmdBuf=1;else cmdBuf=0;
				return;
			}
			command[cmdBuf][cmdPtr++]=data[i];
			if(cmdPtr==256) cmdPtr=0;
			if(x++==dLength) return;
			break;

		default:
			return;
		}
	}
}

static void Parse_Command(char *cmd)
{
	char var[100];
	char val[100];

	//	To lowercase
	for(int i=0;cmd[i]!=0;i++)
	{
		cmd[i]=(char)tolower(cmd[i]);
	}

	if(strstr(cmd, "=")!=NULL)	//	Set command
	{
		int i=0;
		int x=0;
		bool t=0;
		for(;cmd[i]!=0;i++)
		{
			if(cmd[i]=='=')
			{
				var[x++]=0;
				t=1;
				x=0;
			}
			else
			{
				if(t==0)
				{
					var[x++]=cmd[i];
					if(x==100) return;
				}
				else
				{
					val[x++]=cmd[i];
					if(x==100) return;
				}
			}
		}
		val[x++]=0;
		SetCommand(var, val);
	}
	else if(strstr(cmd, "?")!=NULL)	//	Query command
	{
		int i=0;
		int x=0;
		for(;(cmd[i]!=0) && (cmd[i]!='?');i++)
		{
			var[x++]=cmd[i];
			if(x==100) return;
		}
		var[x++]=0;
		QueryCommand(var);

	}
	else	//	Execute command
	{
		int i=0;
		int x=0;
		for(;cmd[i]!=0;i++)
		{
			var[x++]=cmd[i];
			if(x==100) return;
		}
		var[x++]=0;
		ExecuteCommand(var);
	}
}

static void SetCommand(char *var, char *val)
{
	char sbuf[256];
	if(strcmp(var, "debug")==0)
	{
		sprintf(sbuf, "DEBUG=%s\n\r", val);
		SendString(sbuf);
	}
	if(strcmp(var,"rr")==0)	//	Read SPI Register
	{

		char vals[2][30];
		SplitVal(val, vals[0], vals[1]);
		uint8 reg=atoi(vals[0]);
		uint8 cs=atoi(vals[1]);
		sprintf(sbuf, "Reg=%02X\r\nCS=%02X\r\n", reg,cs);
		SendString(sbuf);
		uint8 data=ReadRegister(reg,cs);
		sprintf(sbuf, "%02X\r\n", data);
		SendString(sbuf);
	}
	else if(strcmp(var, "wr")==0)
	{
		char cReg[5];
		char cVal[5];
		char cCS[5];
		char foo[30];

		SplitVal(val, cReg, foo);
		SplitVal(foo, cVal, cCS);
		uint8 reg=atoi(cReg);
		uint8 data=atoi(cVal);
		uint8 cs=atoi(cCS);
		WriteRegister(reg,data,cs);
		SendString("Register Written\r\n");
	}
	else
	{
		sprintf(sbuf, "ERROR:COMMAND NOT RECOGNIZED:%s\r\n", var);
		SendString(sbuf);
	}
}

static void QueryCommand(char *var)
{
	char qbuf[256];
	if(strcmp(var, "debug")==0)
	{
		SendString("DEBUG=ABCD\r\n");
	}
	else
	{
		sprintf(qbuf, "ERROR:COMMAND NOT RECOGNIZED:%s\r\n", var);
		SendString(qbuf);
	}
}

static void ExecuteCommand(char *var)
{
	char ebuf[256];
	if(strcmp(var, "start")==0)
	{
		StartConversions();
	}
	else if(strcmp(var, "eeg")==0)
	{
		Select_EEG();
		SendString("EEG Mode Selected\r\n");
	}
	else if(strcmp(var, "tdcs")==0)
	{
		Select_TDCS();
		SendString("TDCS Mode Selected\r\n");
	}

	else
	{
		sprintf(ebuf, "ERROR:COMMAND NOT RECOGNIZED:%s\r\n", var);
		SendString(ebuf);
	}
}

static bool SplitVal(char *val, char *val1, char *val2)
{
	int i=0;
	bool t=0;
	int x=0;
	for(;val[i]!=0;i++)
	{
		if((t==0) && (val[i]==','))
		{
			val1[x++]=0;
			x=0;
			t=1;
			continue;
		}
		if(t==0) val1[x++]=val[i];else val2[x++]=val[i];
	}
	val2[x++]=0;
	return 0;
}

static void SetBaudFast(void)
{
	UART0_BDL=13;
}


static void SendEEGData(void)
{



	//Set_Led(2,1);
	for(int smp=0;smp<SAMPSIZE;smp++)
	{
		for(int i=0;i<8;i++)
		{
			if(i!=7) sprintf(ibuf, "%06X,", eegData[smp][i]);else sprintf(ibuf, "%06X\r\n", eegData[smp][i]);
			strcat(ebuf,ibuf);
		}
	}
	SendString(ebuf);
	//Set_Led(2,0);
}
#endif

