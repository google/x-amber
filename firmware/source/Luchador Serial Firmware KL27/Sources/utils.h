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
 * utils.h
 *
 *  Created on: Sep 29, 2016
 *      Author: nick
 */

#ifndef SOURCES_UTILS_H_
#define SOURCES_UTILS_H_

#include <stdint.h>


#define DEC(x) if(x>0) x--

typedef union
{
	uint32_t val32;
	uint16_t val16[2];
	uint8_t val8[4];
	struct
	{
		uint32_t	bit0:1;
		uint32_t	bit1:1;
		uint32_t	bit2:1;
		uint32_t	bit3:1;
		uint32_t	bit4:1;
		uint32_t	bit5:1;
		uint32_t	bit6:1;
		uint32_t	bit7:1;

		uint32_t	bit8:1;
		uint32_t	bit9:1;
		uint32_t	bit10:1;
		uint32_t	bit11:1;
		uint32_t	bit12:1;
		uint32_t	bit13:1;
		uint32_t	bit14:1;
		uint32_t	bit15:1;

		uint32_t	bit16:1;
		uint32_t	bit17:1;
		uint32_t	bit18:1;
		uint32_t	bit19:1;
		uint32_t	bit20:1;
		uint32_t	bit21:1;
		uint32_t	bit22:1;
		uint32_t	bit23:1;

		uint32_t	bit24:1;
		uint32_t	bit25:1;
		uint32_t	bit26:1;
		uint32_t	bit27:1;
		uint32_t	bit28:1;
		uint32_t	bit29:1;
		uint32_t	bit30:1;
		uint32_t	bit31:1;

	};
} Type32_u;

void Delay(uint16_t time);
int SplitString(const char *input, char *output, char delim, int offset, uint8_t size);



#endif /* SOURCES_UTILS_H_ */
