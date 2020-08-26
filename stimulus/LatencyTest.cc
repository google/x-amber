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

#include "CommonScreens.h"
#include "Image.h"
#include "Mark.h"
#include "Screen.h"

namespace stimulus {
namespace {

const int kFlashInterval = 500;
const int kTotalFlashes = 30;

class BlackScreen : public Screen {
  SDL_Color GetBackgroundColor() override {
    return SDL_Color{ 0, 0, 0, 0xff };
  }

 public:
  void IsVisible() override {
    SwitchToScreen(0, kFlashInterval);
  }
};

class WhiteScreen : public Screen {
 public:
  void IsActive() override {
    SendMark(10, "WhiteScreen");
  }

  SDL_Color GetBackgroundColor() override {
    return SDL_Color { 0xff, 0xff, 0xff, 0xff };
  }

  void IsVisible() override {
    if (flash_count_++ == kTotalFlashes) {
      flash_count_ = 0;
      SwitchToScreen(1, kFlashInterval);
    } else {
      SwitchToScreen(0, kFlashInterval);
    }
  }

 private:
  int flash_count_ = 0;
};

}  // namespace

Screen *InitLatencyTest(Screen *main_screen) {
  Screen *black = new BlackScreen();
  Screen *white = new WhiteScreen();
  black->AddSuccessor(white);
  white->AddSuccessor(black);
  white->AddSuccessor(main_screen);

  return black;
}

}  // namespace stimulus
