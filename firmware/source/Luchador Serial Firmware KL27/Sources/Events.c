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
**     Filename    : Events.c
**     Project     : Luchador
**     Processor   : MKL26Z256VLH4
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2016-09-27, 12:20, # CodeGen: 0
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Contents    :
**         Cpu_OnNMIINT - void Cpu_OnNMIINT(void);
**
** ###################################################################*/
/*!
** @file Events.c
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup Events_module Events module documentation
**  @{
*/         
/* MODULE Events */

#include "Cpu.h"
#include "Events.h"

#ifdef __cplusplus
extern "C" {
#endif 


/* User includes (#include below this line is not maintained by Processor Expert) */

#include <string.h>
#include <stdio.h>
#include "WiFi/wifi.h"
#include "Serial/serial.h"
#include "settings.h"
#include "version.h"
#include "utils.h"




/*
** ===================================================================
**     Event       :  Cpu_OnReset (module Events)
**
**     Component   :  Cpu [MKL26Z256LH4]
*/
/*!
**     @brief
**         This software event is called after a reset.
**     @param
**         Reason          - Content of the reset status register.
**                           You can use predefined constants RSTSRC_*
**                           defined in generated PE_Const.h file to
**                           determine a reason of the last reset. See
**                           definition of these constants in this file
**                           for details.
*/
/* ===================================================================*/
void Cpu_OnReset(uint16_t Reason)
{
  /* Write your code here ... */
}





#ifdef spiCom1_IDX
/*
** ===================================================================
**     Interrupt handler : SPI0_IRQHandler
**
**     Description :
**         User interrupt service routine. 
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void SPI0_IRQHandler(void)
{
#if spiCom1_DMA_MODE
  SPI_DRV_DmaIRQHandler(spiCom1_IDX);
#else
  SPI_DRV_IRQHandler(spiCom1_IDX);
#endif
  /* Write your code here ... */
}
#else
  /* This IRQ handler is not used by spiCom1 component. The purpose may be
   * that the component has been removed or disabled. It is recommended to 
   * remove this handler because Processor Expert cannot modify it according to 
   * possible new request (e.g. in case that another component uses this
   * interrupt vector). */
  #warning This IRQ handler is not used by spiCom1 component.\
           It is recommended to remove this because Processor Expert cannot\
           modify it according to possible new request.
#endif

#ifdef pitTimer1_IDX
/*
** ===================================================================
**     Interrupt handler : pitTimer1_IRQHandler
**
**     Description :
**         User interrupt service routine. 
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void pitTimer1_IRQHandler(void)
{
  /* Clear interrupt flag.*/
  PIT_HAL_ClearIntFlag(g_pitBase[pitTimer1_IDX], pitTimer1_CHANNEL);
  /* Write your code here ... */
  extern uint16_t tmr_led1;

  extern volatile uint16_t tmr_Serial;

  #ifdef wifi
    extern uint16 tmr_wifi;
    extern uint16 tmr_wgp;
  #endif
  extern uint16_t tmr_delay;
  extern uint16_t tmr_EEGSIM;

    DEC(tmr_led1);

  #ifdef WIFI
    DEC(tmr_wifi);
    DEC(tmr_wgp);
  #endif
    DEC(tmr_delay);
    DEC(tmr_EEGSIM);
    DEC(tmr_Serial);
}
#else
  /* This IRQ handler is not used by pitTimer1 component. The purpose may be
   * that the component has been removed or disabled. It is recommended to 
   * remove this handler because Processor Expert cannot modify it according to 
   * possible new request (e.g. in case that another component uses this
   * interrupt vector). */
  #warning This IRQ handler is not used by pitTimer1 component.\
           It is recommended to remove this because Processor Expert cannot\
           modify it according to possible new request.
#endif

/*
** ===================================================================
**     Interrupt handler : PORTCD_IRQHandler
**
**     Description :
**         User interrupt service routine. 
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void PORTCD_IRQHandler(void)
{
  /* Clear interrupt flag.*/
  PORT_HAL_ClearPortIntFlag(PORTD_BASE_PTR);
  /* Write your code here ... */

}

/*
** ===================================================================
**     Interrupt handler : GpioEeg_PORTA_IRQHandler
**
**     Description :
**         User interrupt service routine. 
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void GpioEeg_PORTA_IRQHandler(void)
{
  /* Clear interrupt flag.*/
  PORT_HAL_ClearPortIntFlag(PORTA_BASE_PTR);
  /* Write your code here ... */
  extern bool drdy;

  drdy=true;
}


/*
** ===================================================================
**     Interrupt handler : ADC0_IRQHandler
**
**     Description :
**         User interrupt service routine. 
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void ADC0_IRQHandler(void)
{
  /* Write your code here ... */
}

/* END Events */

#ifdef __cplusplus
}  /* extern "C" */
#endif 

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
