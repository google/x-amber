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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_MARK_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_MARK_H_

#include <string>

namespace stimulus {

enum MarkFormat {
  kBrainometer,
  kByte,
  kParallel
};

void SendMark(int num, const std::string &event = "undefined");
void SetMarkFormat(MarkFormat format);
void OpenMarkPort(const std::string &portName, int baudRate);
void SetMarkDirectory(const std::string &dir);
void OpenMarkFile(const std::string &task_name);
void CloseMarkFile();

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_MARK_H_
