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
 * leds.c
 *
 *  Created on: Sep 28, 2016
 *      Author: nick
 */

#include "leds.h"
#include "RgbLed.h"
#include "StatusLed.h"

#include "../WiFi/wifi.h"

volatile uint32_t tmr_led1;


void Init_Leds(void)
{

}

void Set_Led(int led, bool state)
{
	GPIO_DRV_WritePinOutput(GPIO_Status, state);
}

bool Get_Led(int led)
{
	return GPIO_DRV_ReadPinInput(GPIO_Status);
}


void Do_Leds(void)
{
	if(tmr_led1!=0) return;
	tmr_led1=100;
	if(Get_Led(1)) Set_Led(1,0); else Set_Led(1,1);
}

void Leds_SetRgb(int rgbMask)
{
	if(rgbMask&0x01) GPIO_DRV_SetPinOutput(LED_R);else GPIO_DRV_ClearPinOutput(LED_R);
	if(rgbMask&0x02) GPIO_DRV_SetPinOutput(LED_G);else GPIO_DRV_ClearPinOutput(LED_G);
	if(rgbMask&0x04) GPIO_DRV_SetPinOutput(LED_B);else GPIO_DRV_ClearPinOutput(LED_B);
}
