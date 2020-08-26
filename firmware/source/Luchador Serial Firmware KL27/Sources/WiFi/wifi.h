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
 * wifi.h
 *
 *  Created on: Sep 28, 2016
 *      Author: nick
 */

#ifndef SOURCES_WIFI_WIFI_H_
#define SOURCES_WIFI_WIFI_H_


#define WIFIBUFSIZE		(256)

void Do_Wifi(void);
void Init_Wifi(void);
void Parse_Wifi(void);
int GetConnectionStatus(void);

#endif /* SOURCES_WIFI_WIFI_H_ */
