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

/* ###################################################################
 **     Filename    : main.c
 **     Project     : Luchador
 **     Processor   : MKL26Z256VLH4
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2016-09-27, 12:20, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 01.01
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "clockMan1.h"
#include "pin_mux.h"
#include "osa1.h"
#include "AuxGpio.h"
#include "dmaController1.h"
#include "MarkUart.h"
#include "Adc.h"
#include "DataUart.h"
#include "spiCom1.h"
#include "RgbLed.h"
#include "StatusLed.h"
#include "GpioEeg.h"
#include "pitTimer1.h"
#include "HardwareVersion.h"
#if CPU_INIT_CONFIG
#include "Init_Config.h"
#endif





/* User includes (#include below this line is not maintained by Processor Expert) */

#include "WiFi/wifi.h"
#include "LEDS/leds.h"
#include "EEG-SPI/spi.h"
#include "EEG-SPI/eeg.h"
#include "Serial/serial.h"
#include "Serial/cli.h"
#include "utils.h"
#include <string.h>
#include <stdint.h>


//#define WIFI
//#define SIMULATE_EEG

//	The location of the checksum in memory
const uint32_t  __attribute__((section (".m_chksum"))) CHKSUM=0;


/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
	/* Write your local variable definition here */

	/*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
	PE_low_level_init();
	/*** End of Processor Expert internal initialization.                    ***/

	/* Write your code here */



	//	Initialization
#ifdef WIFI
	Init_Wifi();
#else
	Init_Serial();
	Serial_SendString("\n\rLuchador Serial CLI\n\r\n\r-->", PORT_MARK);
#endif



	Init_Leds();
	Init_EEG();
	Reset_EEG();
	StartConversions();
	/*
 Reset_EEG();
 StartConversions();
	 */

	ReadADSRegisters();



	Leds_SetRgb(0x2);	//	Green

	while(1)
	{
		Do_Leds();
#ifdef SIMULATE_EEG
		SimulateEEG();
#else
		Do_EEG();

#endif
		Serial_Process();

	}

	/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */


/*!
 ** @}
 */
/*
 ** ###################################################################
 **
 **     This file was created by Processor Expert 10.5 [05.21]
 **     for the Freescale Kinetis series of microcontrollers.
 **
 ** ###################################################################
 */
