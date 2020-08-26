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
 * version.h
 *
 *  Created on: Nov 2, 2018
 *      Author: nick
 */

#ifndef SOURCES_VERSION_H_
#define SOURCES_VERSION_H_
#include <stdint.h>
#include <stdbool.h>

#define SERIAL_SIZE			(32)

uint32_t Version_GetMajor();
uint32_t Version_GetMinor();
void Version_GetVersionString(char *version);
const char* Version_GetBuildNumber();
void Version_GetHwVersionString(char *version);
void Version_GetSerialNumber(char *serial);
uint32_t Version_GetPatch();
bool Version_SetSerialNumber(char *serial);

#endif /* SOURCES_VERSION_H_ */
