// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <cctype>
#include <string>
#include "Settings.h"

namespace stimulus {
namespace {

bool IsIdentifierChar(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
      || (ch >= '0' && ch <= '9') || ch == '_';
}

}  // namespace

Settings::Settings(const std::string &filename) {
  FILE *file = fopen(filename.c_str(), "r");
  if (file == nullptr) {
    errors_ += "Could not open settings file";
    return;
  }

  for (int line_num = 1; ; line_num++) {
    char line_value[128];
    if (fgets(line_value, sizeof(line_value), file) == nullptr) {
      break;
    }

    const char *key_start = line_value;
    while (*key_start && !IsIdentifierChar(*key_start) && *key_start != '#') {
      key_start++;
    }

    if (*key_start == '#') {
        // Ignore comment lines
        continue;
    }

    if (*key_start == '\0') {
      // Blank line
      continue;
    }

    const char *key_end  = key_start + 1;
    while (*key_end && IsIdentifierChar(*key_end)) {
      key_end++;
    }

    if (*key_end == '\0') {
      errors_ += "line ";
      errors_ += std::to_string(line_num) + ": missing value\n";
      continue;
    }

    const char *value_start = key_end + 1;
    while (*value_start &&  isspace(*value_start)) {
      value_start++;
    }

    if (*value_start == '\0') {
      errors_ += "line ";
      errors_ += std::to_string(line_num) + ": missing value\n";
      continue;
    }

    const char *value_end = value_start + 1;
    while (*value_end && !isspace(*value_end)) {
      value_end++;
    }

    std::string key_string(key_start, key_end - key_start);
    std::string value_string(value_start, value_end - value_start);
    values_.insert(std::pair<std::string, std::string>(key_string,
        value_string));
  }
}

const std::string Settings::GetValue(const std::string &key) const {
  std::map<std::string, std::string>::const_iterator it = values_.find(key);
  if (it == values_.end()) {
    return "";
  }

  return it->second;
}

bool Settings::HasKey(const std::string &key) const {
  return values_.find(key) != values_.end();
}

int Settings::GetIntValue(const std::string &key) const {
  std::string value = GetValue(key);
  return std::stoi(value, nullptr, 10);
}

float Settings::GetFloatValue(const std::string &key) const {
  std::string value = GetValue(key);
  return std::stof(value);
}

}  // namespace stimulus
