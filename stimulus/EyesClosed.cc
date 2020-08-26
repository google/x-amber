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
#include "Util.h"

namespace stimulus {
namespace {

const int kMarkInterval = 500;
const int kTaskDuration = 2 * 60 * 1000;

const int kMarkTaskStartStop = 999;

class EyesClosedScreen : public Screen {
 public:
  EyesClosedScreen() {
  }

  void IsActive() override {
    Uint32 now = SDL_GetTicks();
    next_mark_ = now + kMarkInterval;
    task_end_ = now + kTaskDuration;
  }

  void Render() override {
    Uint32 now = SDL_GetTicks();
    if (WrappedGreaterEqual(now, task_end_)) {
      SwitchToScreen(0);  // Back to selection screen
    } else if (WrappedGreaterEqual(now, next_mark_)) {
      SendMark(10, "Marker");
      next_mark_ += kMarkInterval;
    }
  }

 private:
  Uint32 next_mark_;
  Uint32 task_end_;
};

}  // namespace

Screen *InitEyesClosed(Screen *main_screen) {
  Screen *version = new VersionScreen();
  Screen *start = new MarkScreen(kMarkTaskStartStop);
  Screen *eyes_closed = new EyesClosedScreen();
  Screen *finish = new MarkScreen(kMarkTaskStartStop);

  version->AddSuccessor(start);
  start->AddSuccessor(eyes_closed);
  eyes_closed->AddSuccessor(finish);
  finish->AddSuccessor(main_screen);

  return version;
}

}  // namespace stimulus
