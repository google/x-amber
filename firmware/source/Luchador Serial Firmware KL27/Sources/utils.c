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
 * utils.c
 *
 *  Created on: Sep 29, 2016
 *      Author: nick
 */

#include "utils.h"
#include <stdint.h>

volatile uint8_t tmr_delay;


void Delay(uint16_t time)
{
	tmr_delay=time;
	while(tmr_delay!=0);
}


int SplitString(const char *input, char *output, char delim, int offset, uint8_t size)
{
	int count=0;
	int oi=0;

	for(int i=0;input[i]!=0;i++)
	{
		if(input[i]==delim)
		{
			count++;
		}
		else if(count==offset)
		{
			output[oi++]=input[i];
			output[oi]=0;
			if(oi==size) break;
		}
	}
	return count;
}
