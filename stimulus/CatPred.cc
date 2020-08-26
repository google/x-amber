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

#include "CatPred.h"

#include <cassert>
#include <memory>

#include "CatPredItems.h"
#include "CommonScreens.h"
#include "Image.h"
#include "Mark.h"
#include "Random.h"
#include "Shuffler.h"

namespace stimulus {
namespace {

// Counts
const int kTotalTrials = 120;
const int kNumTrialsBeforeBreak = 40;
const int kNumCategories = 3;
const int kMaxRun = 3;

// Timing
const int kMinPrimingFixationTimeMs = 150;
const int kMaxPrimingFixationTimeMs = 350;
const int kTargetFixationTimeMs = 2000;
const int kResponseFixationTimeMs = 3000;
const int kCueTimeMs = 1000;
const int kTargetTimeMs = 1000;
const int kResponseTimeoutMs = 2000;

// Marks
const int kMarkLowTypicality = 200;
const int kMarkHighTypicality = 201;
const int kMarkIncongruent = 202;
const int kMarkResponseYes = 204;
const int kMarkResponseNo = 206;
const int kMarkNoResponse = 299;
const int kMarkTaskStartStop = 999;
const int kMarkOffset = 777;
const int kMarkTaskBreak = 888;

// Keys
const SDL_Scancode kScancodeYes = SDL_SCANCODE_S;
const SDL_Scancode kScancodeNo = SDL_SCANCODE_L;

struct CatPredState {
  Shuffler<int> typicality_shuffler;
  Shuffler<const CatPredItem *> cue_shuffler;
  int trial_count;
  const CatPredItem *next_item;
  const char *next_typicality_string;
  int next_typicality_mark;
};

class CatPredInstructionScreen : public InstructionExamplesScreen {
 public:
  CatPredInstructionScreen(std::shared_ptr<CatPredState> state)
      : InstructionExamplesScreen("", nullptr, nullptr, 0, "Category", "Word"),
        state_(state) {
    int display_width_px = GetDisplayWidthPx();
    int display_height_px = GetDisplayHeightPx();

    line1_location_.x = (display_width_px - GetStringWidth(kLine1)) / 2;
    line1_location_.y =
        ((display_height_px / 2) - GetFontHeight()) / 2 - GetFontHeight();
    line2_location_.x = (display_width_px - GetStringWidth(kLine2)) / 2;
    line2_location_.y = line1_location_.y + GetFontHeight();

    left_example_location_ =
        CenterStringInRect(kLeftExample, GetLeftExampleRect(), 1.0);
    right_example_location_ =
        CenterStringInRect(kRightExample, GetRightExampleRect(), 1.0);

    for (int i = 0; i < kNumCategories; i++) {
      std::vector<int> v(kTotalTrials / kNumCategories);
      std::fill(v.begin(), v.end(), i);
      state_->typicality_shuffler.AddCategoryElements(v, kMaxRun);
    }
    std::vector<const CatPredItem *> v;
    for (int i = 0; i < kTotalTrials; i++) {
      v.push_back(&CatPredItems[i]);
    }
    state_->cue_shuffler.AddCategoryElements(v, 0);
  }

  void IsActive() override {
    // Shuffle elements before task start
    state_->typicality_shuffler.ShuffleElements();
    state_->cue_shuffler.ShuffleElements();
    state_->trial_count = 0;
  }

  void IsInactive() override {
    // ok to send this mark here because it's not time locked
    SendMark(kMarkTaskStartStop);
  }

  void Render() override {
    InstructionExamplesScreen::Render();
    DrawString(line1_location_.x, line1_location_.y, kLine1);
    DrawString(line2_location_.x, line2_location_.y, kLine2);
    DrawString(left_example_location_.x, left_example_location_.y, kLeftExample,
               1.0);
    DrawString(right_example_location_.x, right_example_location_.y,
               kRightExample, 1.0);
  }

 private:
  std::shared_ptr<CatPredState> state_;

  std::string kLine1 = "Indicate if the word is a member of the category";
  std::string kLine2 = "by pressing \"S\" for yes or \"L\" for no";
  std::string kLeftExample = "A kind of tree";
  std::string kRightExample = "Oak";

  SDL_Point line1_location_;
  SDL_Point line2_location_;
  SDL_Point left_example_location_;
  SDL_Point right_example_location_;
};

class CueScreen : public Screen {
 public:
  CueScreen(std::shared_ptr<CatPredState> state) : state_(state) {}

  void IsActive() override {
    assert(!state_->cue_shuffler.IsDone());

    state_->trial_count += 1;
    state_->next_item = state_->cue_shuffler.GetNextItem();
    cue_location_ = CenterString(state_->next_item->cue, 1.0);

    int typicality = state_->typicality_shuffler.GetNextItem();
    switch (typicality) {
      case 0:
        state_->next_typicality_string = state_->next_item->high_typicality;
        state_->next_typicality_mark = kMarkHighTypicality;
        break;
      case 1:
        state_->next_typicality_string = state_->next_item->low_typicality;
        state_->next_typicality_mark = kMarkLowTypicality;
        break;
      case 2:
        state_->next_typicality_string = state_->next_item->incongruent;
        state_->next_typicality_mark = kMarkIncongruent;
        break;
      default:
        SDL_Log("Unexpected typicality value %d", typicality);
        assert(false);
    }

    SwitchToScreen(0, kCueTimeMs);
  }

  void Render() override {
    DrawString(cue_location_.x, cue_location_.y, state_->next_item->cue, 1.0);
  }

  void IsVisible() override { SendMark(state_->next_item->cue_mark); }

 private:
  std::shared_ptr<CatPredState> state_;
  SDL_Point cue_location_;
};

class TargetScreen : public Screen {
 public:
  TargetScreen(std::shared_ptr<CatPredState> state) : state_(state) {}

  void IsActive() override {
    assert(state_->next_typicality_string != nullptr);
    assert(state_->next_typicality_mark != 0);
    target_location_ = CenterString(state_->next_typicality_string, 1.0);
    SwitchToScreen(0, kTargetTimeMs);
  }

  void Render() override {
    DrawString(target_location_.x, target_location_.y,
               state_->next_typicality_string, 1.0);
  }

  void IsVisible() override { SendMark(state_->next_typicality_mark); }

  void IsInactive() override { SendMark(kMarkOffset); }

 private:
  std::shared_ptr<CatPredState> state_;
  SDL_Point target_location_;
};

class ResponseScreen : public Screen {
 public:
  ResponseScreen() { line_location_ = CenterString(kLine, 1.0); }

  void IsActive() override {
    keypressed_ = false;
    SwitchToScreen(0, kResponseTimeoutMs);
  }

  void IsInactive() override {
    if (!keypressed_) {
      SendMark(kMarkNoResponse);
    }
  }

  void Render() override {
    DrawString(line_location_.x, line_location_.y, kLine, 1.0);
  }

  void KeyPressed(SDL_Scancode scode) override {
    switch (scode) {
      case kScancodeYes:
        keypressed_ = true;
        SendMark(kMarkResponseYes);
        break;
      case kScancodeNo:
        keypressed_ = true;
        SendMark(kMarkResponseNo);
        break;
      default:
        break;
    }
    if (keypressed_) {
      SwitchToScreen(0);
    }
  }

 private:
  const std::string kLine = "Yes \"S\"    No \"L\"";
  SDL_Point line_location_;
  bool keypressed_;
};

class ResponseFixationScreen : public FixationDotScreen {
 public:
  ResponseFixationScreen(std::shared_ptr<CatPredState> state, int timeout)
      : FixationDotScreen(timeout, timeout), state_(state) {}

  void IsActive() override {
    if (state_->trial_count == kTotalTrials) {
      next_screen_ = 2;
    } else if ((state_->trial_count % kNumTrialsBeforeBreak) == 0) {
      next_screen_ = 1;
    } else {
      next_screen_ = 0;
    }
  }

  void IsInactive() override {
    // ok to send these marks here as they are not time locked
    if (state_->trial_count == kTotalTrials) {
      SendMark(kMarkTaskStartStop);
    } else if ((state_->trial_count % kNumTrialsBeforeBreak) == 0) {
      SendMark(kMarkTaskBreak);
    }
  }

 private:
  std::shared_ptr<CatPredState> state_;
};

}  // namespace

Screen *InitCatPred(Screen *main_screen, const Settings &settings) {
  std::shared_ptr<CatPredState> state(
      new CatPredState{Shuffler<int>(), Shuffler<const CatPredItem *>(), 0,
                       nullptr, nullptr, 0});

  Screen *instructions = new CatPredInstructionScreen(state);
  Screen *cue = new CueScreen(state);
  Screen *fixation1 = new FixationDotScreen(kMinPrimingFixationTimeMs,
                                            kMaxPrimingFixationTimeMs);
  Screen *target = new TargetScreen(state);
  Screen *fixation2 =
      new FixationDotScreen(kTargetFixationTimeMs, kTargetFixationTimeMs);
  Screen *response = new ResponseScreen();
  Screen *fixation3 =
      new ResponseFixationScreen(state, kResponseFixationTimeMs);
  Screen *rest =
      new InstructionScreen("Take a break! Press any key when ready.");

  instructions->AddSuccessor(cue);
  cue->AddSuccessor(fixation1);
  fixation1->AddSuccessor(target);
  target->AddSuccessor(fixation2);
  fixation2->AddSuccessor(response);
  response->AddSuccessor(fixation3);
  fixation3->AddSuccessor(cue);
  fixation3->AddSuccessor(rest);
  fixation3->AddSuccessor(main_screen);
  rest->AddSuccessor(cue);

  return instructions;
}

}  // namespace stimulus
