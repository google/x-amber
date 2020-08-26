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

#include "WorkingMemory.h"

#include <cassert>
#include <cmath>

#include "CommonScreens.h"
#include "Image.h"
#include "Mark.h"
#include "Random.h"
#include "Shuffler.h"
#include "Version.h"

namespace stimulus {
namespace {

// Trial counts
const int kTotalPracticeTrials = 10;
const int kTotalTrials = 90;
const int kTotalTrialsPerBlock = 30;

// Timing
const int kPracticeMs = 3000;
const int kTrialFixationMs = 2000;
const int kMemoryMs = 100;
const int kMemoryFixationMs = 900;

// Stimuli parameters
const float kCrossWidthCm = 0.4;
const float kStimuliHeightCm = 0.7;
const float kStimuliRadiusCm = 3.1;
const int kNumStimuli = 4;

// Marks
const int kMarkPracticeStartStop = 111;
const int kMarkTaskStartStop = 999;
const int kMarkBlockStart = 888;
const int kMarkMemoryOnset = 5;
const int kMarkMemoryOffset = 777;
const int kMarkRecallLowerRightChanged = 1;
const int kMarkRecallLowerLeftChanged = 2;
const int kMarkRecallUpperLeftChanged = 3;
const int kMarkRecallUpperRightChanged = 4;
// for the two marks below add the changed mark values above to generate
// the actual mark value, e.g. correct response when the lower right square
// is changed uses the mark 10 + 1 = 11
const int kMarkCorrect = 10;
const int kMarkError = 20;

struct State {
  SDL_Rect rect[kNumStimuli];
  SDL_Color color[kNumStimuli];
  SDL_Color changed_color;
  int num_trials;
  int num_correct_trials;
};

const std::vector<SDL_Color> ColorList = {
    {
        /* .r = */ 255,
        /* .g = */ 0,
        /* .b = */ 0,
        /*. a = */ 255,
    },
    {
        /* .r = */ 0,
        /* .g = */ 255,
        /* .b = */ 0,
        /*. a = */ 255,
    },
    {
        /* .r = */ 255,
        /* .g = */ 255,
        /* .b = */ 0,
        /*. a = */ 255,
    },
    {
        /* .r = */ 255,
        /* .g = */ 0,
        /* .b = */ 255,
        /*. a = */ 255,
    },
    {
        /* .r = */ 0,
        /* .g = */ 255,
        /* .b = */ 255,
        /*. a = */ 255,
    },
    {
        /* .r = */ 80,
        /* .g = */ 60,
        /* .b = */ 255,
        /*. a = */ 255,
    },
};

class PracticeScreen : public Screen {
 public:
  PracticeScreen(std::shared_ptr<State> state) : state_(state) {
    instruction_location_.x =
        (GetDisplayWidthPx() - GetStringWidth(kInstructions)) / 2;
    instruction_location_.y = (GetDisplayHeightPx() - GetFontHeight()) / 2;
  }

  void IsActive() override {
    state_->num_trials = 0;
    state_->num_correct_trials = 0;
    SendMark(kMarkPracticeStartStop);
    SwitchToScreen(0, kPracticeMs);
  }

  void Render() override {
    DrawString(instruction_location_.x, instruction_location_.y, kInstructions);
  }

 private:
  static const std::string kInstructions;
  SDL_Point instruction_location_;
  std::shared_ptr<State> state_;
};

const std::string PracticeScreen::kInstructions = "PRACTICE...";

class PracticeResultsScreen : public Screen {
 public:
  PracticeResultsScreen(std::shared_ptr<State> state) : state_(state) {
    SetCursorVisible(true);
  }

  void IsActive() override {
    int display_width_px = GetDisplayWidthPx();
    int display_height_px = GetDisplayHeightPx();

    accuracy_str_ =
        "Accuracy: " + std::to_string(state_->num_correct_trials * 10) + "%";
    accuracy_location_.x =
        (display_width_px - GetStringWidth(accuracy_str_)) / 2;
    accuracy_location_.y = ((display_height_px / 2) - GetFontHeight()) / 2;

    int repeat_width = GetStringWidth(kRepeatLabel);
    int start_width = GetStringWidth(kStartLabel);
    int rect_width =
        ((repeat_width > start_width) ? repeat_width : start_width) +
        4 * label_padding;
    repeat_rect_.w = start_rect_.w = rect_width;
    repeat_rect_.h = start_rect_.h = GetFontHeight() + 2 * label_padding;
    repeat_rect_.x = (display_width_px / 2 - repeat_rect_.w) / 2;
    start_rect_.x = (display_width_px / 2) + repeat_rect_.x;
    repeat_rect_.y = start_rect_.y =
        (display_height_px / 2) + (display_height_px / 4 - repeat_rect_.h);

    repeat_location_ = CenterStringInRect(kRepeatLabel, repeat_rect_);
    start_location_ = CenterStringInRect(kStartLabel, start_rect_);

    int center_x = static_cast<int>(
        static_cast<float>(GetDisplayWidthPx() / 2) / GetPixelRatio());
    int center_y = static_cast<int>(
        static_cast<float>(GetDisplayHeightPx() / 2) / GetPixelRatio());
    SDL_WarpMouseGlobal(center_x, center_y);

    // Reset state for experiment trials
    state_->num_trials = 0;
    // Unused for experiment
    state_->num_correct_trials = 0;

    SendMark(kMarkPracticeStartStop);
  }

  void Render() override {
    DrawString(accuracy_location_.x, accuracy_location_.y, accuracy_str_);
    DrawString(repeat_location_.x, repeat_location_.y, kRepeatLabel);
    DrawString(start_location_.x, start_location_.y, kStartLabel);
    DrawRect(repeat_rect_, GetLineColor(), GetBackgroundColor());
    DrawRect(start_rect_, GetLineColor(), GetBackgroundColor());
  }

  void MouseClicked(int button, int x, int y) override {
    if (IsHit(x, y, start_rect_)) {
      SwitchToScreen(0);
    } else if (IsHit(x, y, repeat_rect_)) {
      SwitchToScreen(1);
    }
  }

 private:
  static const std::string kRepeatLabel;
  static const std::string kStartLabel;
  static const int label_padding = 20;

  std::shared_ptr<State> state_;

  std::string accuracy_str_;
  SDL_Point accuracy_location_;
  SDL_Point repeat_location_;
  SDL_Point start_location_;
  SDL_Rect repeat_rect_;
  SDL_Rect start_rect_;
};

const std::string PracticeResultsScreen::kRepeatLabel = "Repeat Practice";
const std::string PracticeResultsScreen::kStartLabel = "Start Task";

class MemoryScreen : public Screen {
 public:
  MemoryScreen(std::shared_ptr<State> state) : state_(state) {
    cross_ = LoadImage(GetResourceDir() + "cross.bmp");
    cross_rect_ = ComputeRectForPhysicalWidth(cross_, kCrossWidthCm);
    shuffler_.AddCategoryElements(ColorList, 0);
  }

  void IsActive() override {
    shuffler_.ShuffleElements();

    int origin_x = GetDisplayWidthPx() / 2;
    int origin_y = GetDisplayHeightPx() / 2;

    for (int i = 0; i < kNumStimuli; i++) {
      // calculate position relative to center of screen
      // must not be +/- 15 degrees of X/Y axis
      uint32_t deg = GenerateRandomInt(15, 76) + i * 90;
      float rad = static_cast<float>(deg) * M_PI / 180;
      float offset_cm = kStimuliHeightCm / 2;
      float x_cm = kStimuliRadiusCm * std::cos(rad) - offset_cm;
      float y_cm = kStimuliRadiusCm * std::sin(rad) - offset_cm;

      // convert to screen pixels
      int x_px = origin_x + HorzSizeToPixels(x_cm);
      int y_px = origin_y + VertSizeToPixels(y_cm);

      // pick a color
      SDL_Color color = shuffler_.GetNextItem();

      SDL_Rect rect;
      rect.x = x_px;
      rect.y = y_px;
      rect.w = HorzSizeToPixels(kStimuliHeightCm);
      rect.h = VertSizeToPixels(kStimuliHeightCm);

      state_->color[i] = color;
      state_->rect[i] = rect;
    }
    // the color to replace
    state_->changed_color = shuffler_.GetNextItem();
    SwitchToScreen(0, kMemoryMs);
  }

  void Render() override {
    for (int i = 0; i < kNumStimuli; i++) {
      FillRect(state_->rect[i], state_->color[i], GetBackgroundColor());
    }
    Blit(cross_, cross_rect_);
  }

  void IsVisible() override { SendMark(kMarkMemoryOnset); }

  void IsInvisible() override { SendMark(kMarkMemoryOffset); }

 private:
  SDL_Texture *cross_;
  SDL_Rect cross_rect_;
  Shuffler<SDL_Color> shuffler_;
  std::shared_ptr<State> state_;
};

class RecallScreen : public Screen {
 public:
  RecallScreen(std::shared_ptr<State> state, bool practice)
      : state_(state), practice_(practice) {
    cross_ = LoadImage(GetResourceDir() + "cross.bmp");
    cross_rect_ = ComputeRectForPhysicalWidth(cross_, kCrossWidthCm);

    SetCursorVisible(true);
  }

  void IsActive() override {
    changed_index_ = static_cast<int>(GenerateRandomInt(0, kNumStimuli));

    int center_x = static_cast<int>(
        static_cast<float>(GetDisplayWidthPx() / 2) / GetPixelRatio());
    int center_y = static_cast<int>(
        static_cast<float>(GetDisplayHeightPx() / 2) / GetPixelRatio());
    SDL_WarpMouseGlobal(center_x, center_y);
  }

  void Render() override {
    for (int i = 0; i < kNumStimuli; i++) {
      SDL_Color color =
          i == changed_index_ ? state_->changed_color : state_->color[i];
      FillRect(state_->rect[i], color, GetBackgroundColor());
    }
    Blit(cross_, cross_rect_);
  }

  void IsVisible() override {
    switch (changed_index_) {
      case 0:
        mark_ = kMarkRecallLowerRightChanged;
        break;
      case 1:
        mark_ = kMarkRecallLowerLeftChanged;
        break;
      case 2:
        mark_ = kMarkRecallUpperLeftChanged;
        break;
      case 3:
        mark_ = kMarkRecallUpperRightChanged;
        break;
      default:
        assert(false);  // should never get here
    }
    SendMark(mark_);
  }

  void MouseClicked(int button, int x, int y) override {
    int i = 0;
    for (i = 0; i < kNumStimuli; i++) {
      if (IsHit(x, y, state_->rect[i])) {
        break;
      }
    }
    if (i < kNumStimuli) {
      state_->num_trials += 1;
      if (i == changed_index_) {
        state_->num_correct_trials += 1;
        SendMark(kMarkCorrect + mark_);
      } else {
        SendMark(kMarkError + mark_);
      }
      if (practice_) {
        if (state_->num_trials == kTotalPracticeTrials) {
          SwitchToScreen(1);
        } else {
          SwitchToScreen(0);
        }
      } else {
        if (state_->num_trials == kTotalTrials) {
          SwitchToScreen(2);
        } else if ((state_->num_trials % kTotalTrialsPerBlock) == 0) {
          SwitchToScreen(1);
        } else {
          SwitchToScreen(0);
        }
      }
    }
  }

 private:
  SDL_Texture *cross_;
  SDL_Rect cross_rect_;
  int changed_index_;
  std::shared_ptr<State> state_;
  bool practice_;
  int mark_;
};

}  // namespace

Screen *InitWorkingMemory(Screen *main_screen, const Settings &Settings) {
  std::shared_ptr<State> state(new State());
  // version
  Screen *version = new VersionScreen();
  // instructions
  Screen *instructions1 = new MultiLineScreen(
      {"In this task you will see several squares appear then disappear on the "
       "screen. Soon the squares will reappear, and you will have to decide "
       "which of the squares has changed colors.",
       "", "Click the mouse to continue."});
  Screen *instructions2 = new MultiLineScreen(
      {"First, we will do some practice.", "Click the mouse to continue."});
  // practice
  Screen *practice = new PracticeScreen(state);
  Screen *p_f =
      new FixationScreen(kTrialFixationMs, kTrialFixationMs, kCrossWidthCm);
  Screen *p_memory = new MemoryScreen(state);
  Screen *p_mf =
      new FixationScreen(kMemoryFixationMs, kMemoryFixationMs, kCrossWidthCm);
  Screen *p_recall = new RecallScreen(state, true);
  Screen *p_results = new PracticeResultsScreen(state);
  Screen *p_finish = new MultiLineScreen(
      {"You have now completed the practice.", "Click the mouse to continue."},
      kMarkTaskStartStop);
  // experiment
  Screen *start = new MultiLineScreen(
      {"Starting a new block of trials...", "Click the mouse to begin."},
      kMarkBlockStart);
  Screen *f =
      new FixationScreen(kTrialFixationMs, kTrialFixationMs, kCrossWidthCm);
  Screen *memory = new MemoryScreen(state);
  Screen *mf =
      new FixationScreen(kMemoryFixationMs, kMemoryFixationMs, kCrossWidthCm);
  Screen *recall = new RecallScreen(state, false);
  Screen *finish = new MultiLineScreen(
      {"This is the end of the experiment.", "Thank you!"}, kMarkTaskStartStop);

  version->AddSuccessor(instructions1);
  instructions1->AddSuccessor(instructions2);
  instructions2->AddSuccessor(practice);
  practice->AddSuccessor(p_f);
  p_f->AddSuccessor(p_memory);
  p_memory->AddSuccessor(p_mf);
  p_mf->AddSuccessor(p_recall);
  p_recall->AddSuccessor(p_f);
  p_recall->AddSuccessor(p_results);
  p_results->AddSuccessor(p_finish);
  p_results->AddSuccessor(practice);
  p_finish->AddSuccessor(start);
  start->AddSuccessor(f);
  f->AddSuccessor(memory);
  memory->AddSuccessor(mf);
  mf->AddSuccessor(recall);
  recall->AddSuccessor(f);
  recall->AddSuccessor(start);
  recall->AddSuccessor(finish);
  finish->AddSuccessor(main_screen);

  return version;
}

}  // namespace stimulus
