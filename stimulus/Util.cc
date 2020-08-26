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

#include "Util.h"

#include <cassert>

namespace stimulus {
namespace {

void PushString(std::vector<RenderString> &out_vec, std::string &str, int y) {
  RenderString instr = {
      /*.str = */ str,
      /*.x = */ 0,
      /*.y = */ y,
  };
  out_vec.push_back(instr);
}

}  // namespace

int LineBreak(std::vector<RenderString> &out_vec, std::string str,
              unsigned max_chars, int y, int font_height) {
  int count = 0;
  while (str.length() > max_chars) {
    // try to linebreak at a space
    std::string::size_type i = str.rfind(' ', max_chars - 1);
    if (i == std::string::npos) {
      // no space characters found, break at the max length
      i = max_chars;
    } else {
      i += 1;
    }
    std::string sub = str.substr(0, i);
    assert(sub.length() > 0);
    PushString(out_vec, sub, y);
    y += font_height;
    count += 1;
    str = str.substr(i);
  }
  // last segment
  PushString(out_vec, str, y);
  count += 1;
  return count;
}

}  // namespace stimulus
