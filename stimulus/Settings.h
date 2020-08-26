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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SETTINGS_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SETTINGS_H_

#include <string>
#include <map>

namespace stimulus {

class Settings {
 public:
  Settings(const std::string &filename);
  const std::string GetValue(const std::string &key) const;
  int GetIntValue(const std::string &key) const;
  float GetFloatValue(const std::string &key) const;
  std::string GetErrors() const {
    return errors_;
  }

  bool HasKey(const std::string&) const;

 private:
  std::map<std::string, std::string> values_;
  std::string errors_;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SETTINGS_H_
