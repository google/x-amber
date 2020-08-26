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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_PLATFORM_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_PLATFORM_H_

#include <vector>
#include <string>

namespace stimulus {

int OpenParallel(const std::string &portName);
void WriteParallel(int datum);
int OpenSerial(const std::string &name, int baud_rate);
void CloseSerial();
int WriteSerial(const void *buf, int length);
int ReadSerial(void *buf, int length);
std::string GetResourceDir();
uint32_t GetRandomSeed();
std::vector<std::string> GetAvailableSerialPorts();

struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

DateTime GetDateTime();

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_PLATFORM_H_
