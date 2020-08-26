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

#include "HotButton.h"

#include <cassert>
#include <memory>

#include "CommonScreens.h"
#include "HotButtonEngine.h"
#include "Image.h"
#include "Mark.h"
#include "Random.h"
#include "TextureManager.h"
#include "Util.h"

namespace stimulus {
namespace {

// Instruction strings
const std::string kInstructionsScreen2[] = {
    "The goal of this game is to win as many points as possible. Win points by "
    "completing a button pressing task.",
    "",
    "For each task, you can select Easy or Hard:",
    "",
    "Easy task: Press [DOMINANT_HAND_KEY] 21 times in 5 seconds. Use your "
    "[DOMINANT_HAND] index finger.",
    "",
    "Hard task: Press [NON_DOMINANT_HAND_KEY] 71 times in 15 seconds. Use "
    "your [NON_DOMINANT_HAND] pinky finger."};
int kNumInstructionsScreen2 = 7;
const std::string kInstructionsScreen3[] = {
    "At the start of each task, you will be told how many points you can win "
    "and how likely you are to win them if you complete the task.",
    "",
    "You will always be able to win more points in the hard tasks than the "
    "easy tasks."};
int kNumInstructionsScreen3 = 3;
const std::string kInstructionsScreen4[] = {
    "At the end of each task, you will see dice as the computer decides if "
    "you've won points.",
    "",
    "Then you will find out if you've won points and how many points you've "
    "won."};
int kNumInstructionsScreen4 = 3;
const std::string kInstructionsScreen5[] = {
    "You will also see how many points you've won total out of the maximum "
    "points possible.",
    "",
    "Try to win as many points as you can! You can always win more points in "
    "the hard task."};
int kNumInstructionsScreen5 = 3;
const std::string kInstructionsScreen6[] = {
    "This game lasts exactly 20 minutes, after which you will be asked to "
    "stop. Even if you always choose the easy task, which is shorter, you will "
    "still play for a full 20 minutes.",
    "",
    "You can always win more points in the hard task. Try to earn as many "
    "points as you can!"

};
int kNumInstructionsScreen6 = 3;

int kTextTop = 200;
int kTextLeft = 400;

const float kFontScale = 1.6;
const int kNumDialImages = 31;
const SDL_Color kDimColor = SDL_Color{0x7b, 0x7b, 0x7b, 0xff};

// Miscellaneous strings
const std::string kReady = "Ready";

// Timing
const int kTotalTimeMs = 20 * 60 * 1000;
const int kReadyTimeMs = 500;
const int kPretaskTimeS = 30;
const int kPretaskTimeMs = kPretaskTimeS * 1000;
const int kPretaskFinishTimeMs = 1000;
const int kMinFixationTimeMs = 400;
const int kMaxFixationTimeMs = 600;
const int kResultTimeMs = 750;
const int kDiceTimeMs = 750;
const int kFeedbackTimeMs = 1750;

// Marks
const int kMarkTaskStartStop = 999;
const int kMarkHandednessRight = 1;
const int kMarkHandednessLeft = 2;
const int kMarkBaselineStart = 3;
const int kMarkBaselineKeypress = 4;
const int kMarkBaselineEnd = 5;
const int kMarkCondition12_300 = 6;
const int kMarkCondition12_500 = 7;
const int kMarkCondition50_300 = 8;
const int kMarkCondition50_500 = 9;
const int kMarkCondition88_300 = 10;
const int kMarkCondition88_500 = 11;
const int kMarkTaskEasy = 12;
const int kMarkTaskHard = 13;
const int kMarkTimerStart = 14;
const int kMarkKeypress = 15;
const int kMarkTimerStop = 16;
const int kMarkTrialSuccess = 17;
const int kMarkTrialFailure = 18;
const int kMarkDice = 19;
const int kMarkFeedback0 = 20;
const int kMarkFeedback100 = 21;
const int kMarkFeedback300 = 22;
const int kMarkFeedback500 = 23;
const int kMarkSummary = 888;

// Scancodes
const SDL_Scancode kScancodeRight = SDL_SCANCODE_L;
const SDL_Scancode kScancodeLeft = SDL_SCANCODE_S;

class HotButtonInstructionScreen : public Screen {
 public:
  HotButtonInstructionScreen(const std::string instructions[],
                             int num_instructions) {
    Layout(instructions, num_instructions);
  }

  void Render() override {
    for (auto &instr : instructions_) {
      DrawString(instr.x, instr.y, instr.str);
    }
  }

  void KeyPressed(SDL_Scancode) override { SwitchToScreen(0); }

 protected:
  HotButtonInstructionScreen() {}

  void Layout(const std::string instructions[], int num_instructions) {
    instructions_.clear();
    // layout the text vertically, and line break long lines
    int display_width = GetDisplayWidthPx();
    int font_height = GetFontHeight();
    // fixed-width font so one font_width is fine
    int font_width = GetStringWidth("_");
    unsigned max_chars = (display_width - kTextLeft * 2) / font_width;
    int y = 0;
    for (int i = 0; i < num_instructions; i++) {
      auto &str = instructions[i];
      // blank line
      if (str == "") {
        y += font_height;
      } else {
        int nlines =
            LineBreak(instructions_, str, max_chars, y, font_height);
        y += nlines * font_height;
      }
    }
    // center text
    int yoffset = kTextTop + (GetDisplayHeightPx() - y - kTextTop * 2) / 2;
    // truncate if there are too many lines to fit on the display.
    // just draw past the screen boundary
    if (yoffset < 0) {
      yoffset = kTextTop;
    }
    for (auto &instr : instructions_) {
      instr.x = kTextLeft;
      instr.y += yoffset;
    }
  }

 private:
  std::vector<RenderString> instructions_;
};

const std::string kStringReplacementKeys[] = {
    "[DOMINANT_HAND]",
    "[DOMINANT_HAND_KEY]",
    "[NON_DOMINANT_HAND]",
    "[NON_DOMINANT_HAND_KEY]",
};
const std::string kStringReplacementValuesRight[] = {
    "right",
    "\"L\"",
    "left",
    "\"S\"",
};
const std::string kStringReplacementValuesLeft[] = {
    "left",
    "\"S\"",
    "right",
    "\"L\"",
};
int kNumStringReplacements = 4;

class HotButtonHandedInstructionScreen : public HotButtonInstructionScreen {
 public:
  HotButtonHandedInstructionScreen(std::shared_ptr<HotButtonEngine> engine,
                                   const std::string instructions[],
                                   int num_instructions)
      : HotButtonInstructionScreen(),
        engine_(engine),
        instructions_str_(instructions),
        num_instructions_(num_instructions) {}

  void IsActive() override {
    // Replace strings according to handedness
    const std::string *values = kStringReplacementValuesRight;
    if (engine_->GetLeftHandedness()) {
      values = kStringReplacementValuesLeft;
    }

    std::vector<std::string> instructions;
    for (int i = 0; i < num_instructions_; i++) {
      auto str = instructions_str_[i];
      for (int j = 0; j < kNumStringReplacements; j++) {
        if (str.find(kStringReplacementKeys[j]) != std::string::npos) {
          str = Replace(str, kStringReplacementKeys[j], values[j]);
        }
      }
      instructions.push_back(str);
    }
    Layout(instructions.data(), instructions.size());
  }

 private:
  std::string Replace(std::string &src, const std::string &a,
                      const std::string &b) {
    std::string::size_type i = src.find(a);
    assert(i != std::string::npos);
    return src.substr(0, i) + b + src.substr(i + a.length());
  }

  std::shared_ptr<HotButtonEngine> engine_;
  const std::string *instructions_str_;
  int num_instructions_;
};

// This screen draws a single line of text to the screen and switches to
// the next screen after a timeout.
class TimeoutScreen : public Screen {
 public:
  TimeoutScreen(std::string text, int timeout)
      : text_(text), timeout_(timeout) {
    text_location_ = CenterString(text_, kFontScale);
  }

  void IsActive() override { SwitchToScreen(0, timeout_); }

  void Render() override {
    DrawString(text_location_.x, text_location_.y, text_, kFontScale);
  }

 private:
  std::string text_;
  SDL_Point text_location_;
  int timeout_;
};

const std::string kLeftString1 = "Left";
const std::string kLeftString2 = "Press \"S\"";
const std::string kRightString1 = "Right";
const std::string kRightString2 = "Press \"L\"";

// This screen asks the participant to choose their dominant hand.
class HandednessScreen : public InstructionExamplesScreen {
 public:
  HandednessScreen(std::shared_ptr<HotButtonEngine> engine,
                   std::shared_ptr<HotButtonEngine> demo_engine)
      : InstructionExamplesScreen(
            "Do you prefer to use your left hand or your right hand?"),
        engine_(engine),
        demo_engine_(demo_engine) {
    SDL_Rect left_rect = GetLeftExampleRect();
    CenterInRect(kLeftString1, kLeftString2, left_rect, left_location_1_,
                 left_location_2_);
    SDL_Rect right_rect = GetRightExampleRect();
    CenterInRect(kRightString1, kRightString2, right_rect, right_location_1_,
                 right_location_2_);
  }

  void Render() override {
    InstructionExamplesScreen::Render();
    DrawString(left_location_1_.x, left_location_1_.y, kLeftString1);
    DrawString(left_location_2_.x, left_location_2_.y, kLeftString2);
    DrawString(right_location_1_.x, right_location_1_.y, kRightString1);
    DrawString(right_location_2_.x, right_location_2_.y, kRightString2);
  }

  void IsVisible() override { SendMark(kMarkTaskStartStop); }

  void KeyPressed(SDL_Scancode scode) override {
    switch (scode) {
      case SDL_SCANCODE_S:
        engine_->SetLeftHandedness(true);
        demo_engine_->SetLeftHandedness(true);
        SendMark(kMarkHandednessLeft);
        SwitchToScreen(0);
        break;
      case SDL_SCANCODE_L:
        engine_->SetLeftHandedness(false);
        demo_engine_->SetLeftHandedness(false);
        SendMark(kMarkHandednessRight);
        SwitchToScreen(0);
        break;
      default:
        break;
    }
  }

 private:
  void CenterInRect(const std::string &str1, const std::string &str2,
                    SDL_Rect rect, SDL_Point &loc1, SDL_Point &loc2) {
    int font_height = GetFontHeight();
    int width1 = GetStringWidth(str1);
    loc1.x = rect.x + (rect.w - width1) / 2;
    loc1.y = rect.y + rect.h / 2 - font_height;
    int width2 = GetStringWidth(str2);
    loc2.x = rect.x + (rect.w - width2) / 2;
    loc2.y = loc1.y + font_height;
  }

  std::shared_ptr<HotButtonEngine> engine_;
  std::shared_ptr<HotButtonEngine> demo_engine_;
  SDL_Point left_location_1_;
  SDL_Point left_location_2_;
  SDL_Point right_location_1_;
  SDL_Point right_location_2_;
};

const std::string kInstructionStringLeft =
    "Press the \"S\" key with your left index finger as many times as you can "
    "in 30 seconds";
const std::string kInstructionStringRight =
    "Press the \"L\" key with your right index finger as many times as you can "
    "in 30 seconds";

// This screen gives the instructions for the pretask.
class PretaskInstructionScreen : public Screen {
 public:
  PretaskInstructionScreen(std::shared_ptr<HotButtonEngine> engine)
      : engine_(engine) {
    instruction_left_location_ = CenterString(kInstructionStringLeft);
    instruction_right_location_ = CenterString(kInstructionStringRight);
  }

  void Render() override {
    if (engine_->GetLeftHandedness()) {
      DrawString(instruction_left_location_.x, instruction_left_location_.y,
                 kInstructionStringLeft);
    } else {
      DrawString(instruction_right_location_.x, instruction_right_location_.y,
                 kInstructionStringRight);
    }
  }

  void KeyPressed(SDL_Scancode) override { SwitchToScreen(0); }

  void MouseClicked(int button, int x, int y) override { SwitchToScreen(0); }

 private:
  std::shared_ptr<HotButtonEngine> engine_;
  SDL_Point instruction_left_location_;
  SDL_Point instruction_right_location_;
};

// This screen runs the pretask.
class PretaskScreen : public Screen {
 public:
  PretaskScreen(std::shared_ptr<TextureManager> texture_manager,
                std::shared_ptr<HotButtonEngine> engine)
      : texture_manager_(texture_manager), engine_(engine) {
    assert(kNumDialImages >= kPretaskTimeS);
    for (int i = 0; i < kNumDialImages; i++) {
      char filename[16];
      std::snprintf(filename, sizeof(filename), "dial%02d.svg", i);
      dial_[i] = texture_manager_->LoadImage(GetResourceDir() + filename);
    }
    countdown_location_1_ = CenterString("0", kFontScale);
    countdown_location_2_ = CenterString("00", kFontScale);
  }

  void IsActive() override {
    start_time_ = 0;
    SwitchToScreen(0, kPretaskTimeMs);
  }

  void Render() override {
    int countdown = 0;
    if (start_time_ == 0) {
      start_time_ = SDL_GetTicks();
      countdown = kPretaskTimeS;
    } else {
      countdown = kPretaskTimeS - ((SDL_GetTicks() - start_time_) / 1000);
    }
    if (countdown < 0) {
      countdown = 0;
    }
    if (countdown > kPretaskTimeS) {
      countdown = kPretaskTimeS;
    }
    if (countdown >= 10) {
      DrawString(countdown_location_2_.x, countdown_location_2_.y,
                 std::to_string(countdown), kFontScale);
    } else {
      DrawString(countdown_location_1_.x, countdown_location_1_.y,
                 std::to_string(countdown), kFontScale);
    }
    Blit(dial_[countdown]);
  }

  void IsVisible() override { SendMark(kMarkBaselineStart); }

  void KeyPressed(SDL_Scancode scode) override {
    SDL_Scancode next_scode =
        engine_->GetLeftHandedness() ? kScancodeLeft : kScancodeRight;
    if (scode == next_scode) {
      SendMark(kMarkBaselineKeypress);
    }
  }

 private:
  std::shared_ptr<TextureManager> texture_manager_;
  std::shared_ptr<HotButtonEngine> engine_;

  SDL_Texture *dial_[kNumDialImages];
  SDL_Point countdown_location_1_;
  SDL_Point countdown_location_2_;

  int start_time_;
};

// This is a blank screen. The purpose is to timestamp the beginning of the
// set of trials.
class PreloadScreen : public Screen {
 public:
  PreloadScreen(std::shared_ptr<HotButtonEngine> engine) : engine_(engine) {}

  void IsActive() override {
    engine_->Reset();
    engine_->SetStartTime(SDL_GetTicks());
    SwitchToScreen(0);
  }

 private:
  std::shared_ptr<HotButtonEngine> engine_;
};

const int kConditionNumDialImages = 3;
const int kConditionTextMargin = 100;

const std::string kHardTaskString = "Hard Task";
const std::string kEasyTaskString = "Easy Task";
const std::string kRightKeyString = " (L)";
const std::string kLeftKeyString = " (S)";
const std::string kPointsString = " Points";

// This screen shows the probabilities and points for the next trial,
// and allows the participant to choose between the easy or hard task.
class ConditionScreen : public Screen {
 public:
  ConditionScreen(std::shared_ptr<TextureManager> texture_manager,
                  std::shared_ptr<HotButtonEngine> engine)
      : texture_manager_(texture_manager), engine_(engine) {
    dial_[0] = texture_manager_->LoadImage(GetResourceDir() + "dial03.svg");
    dial_[1] = texture_manager_->LoadImage(GetResourceDir() + "dial16.svg");
    dial_[2] = texture_manager_->LoadImage(GetResourceDir() + "dial26.svg");
    win_percentage_location_ = CenterString("12%", kFontScale);

    int dial_width;
    int dial_height;
    SDL_QueryTexture(dial_[0], nullptr, nullptr, &dial_width, &dial_height);

    SDL_Rect dial_rect = CenterRect(dial_width, dial_height);
    int display_height = GetDisplayHeightPx();
    int font_height = GetFontHeight(kFontScale);
    int font_width = GetStringWidth("_", kFontScale);

    int task_points_width =
        GetStringWidth(kHardTaskString + kRightKeyString, kFontScale);

    // Lots of hard coded assumptions based on string length below...
    left_task_location_.x =
        dial_rect.x - kConditionTextMargin - task_points_width;
    left_task_location_.y = display_height / 2 - font_height;
    left_task_points_location_.x = left_task_location_.x + font_width * 3 / 2;
    ;
    left_task_points_location_.y = left_task_location_.y + font_height;

    right_task_location_.x = dial_rect.x + dial_rect.w + kConditionTextMargin;
    right_task_location_.y = left_task_location_.y;
    right_task_points_location_.x = right_task_location_.x + font_width * 3 / 2;
    right_task_points_location_.y = right_task_location_.y + font_height;
  }

  void IsActive() override {
    next_trial_ = engine_->GetNextTrial();
    next_win_percentage_ =
        std::to_string(next_trial_.win_probability_pct) + "%";

    if (engine_->GetLeftHandedness()) {
      next_scode_easy_ = kScancodeLeft;
      next_scode_hard_ = kScancodeRight;
      left_task_ = kEasyTaskString + kLeftKeyString;
      right_task_ = kHardTaskString + kRightKeyString;
      left_task_points_ =
          std::to_string(next_trial_.easy_points) + kPointsString;
      right_task_points_ =
          std::to_string(next_trial_.hard_points) + kPointsString;
    } else {
      next_scode_easy_ = kScancodeRight;
      next_scode_hard_ = kScancodeLeft;
      right_task_ = kEasyTaskString + kRightKeyString;
      left_task_ = kHardTaskString + kLeftKeyString;
      right_task_points_ =
          std::to_string(next_trial_.easy_points) + kPointsString;
      left_task_points_ =
          std::to_string(next_trial_.hard_points) + kPointsString;
    }
  }

  void Render() override {
    switch (next_trial_.win_probability_pct) {
      case kHotButtonWinPercent12:
        Blit(dial_[0]);
        break;
      case kHotButtonWinPercent50:
        Blit(dial_[1]);
        break;
      case kHotButtonWinPercent88:
        Blit(dial_[2]);
        break;
      default:
        break;
    }
    DrawString(win_percentage_location_.x, win_percentage_location_.y,
               next_win_percentage_, kFontScale);
    DrawString(left_task_location_.x, left_task_location_.y, left_task_,
               kFontScale);
    DrawString(right_task_location_.x, right_task_location_.y, right_task_,
               kFontScale);
    DrawString(left_task_points_location_.x, left_task_points_location_.y,
               left_task_points_, kFontScale);
    DrawString(right_task_points_location_.x, right_task_points_location_.y,
               right_task_points_, kFontScale);
  }

  void IsVisible() override {
    if (next_trial_.win_probability_pct == kHotButtonWinPercent12) {
      if (next_trial_.hard_points == kHotButtonPoints300) {
        SendMark(kMarkCondition12_300);
      } else if (next_trial_.hard_points == kHotButtonPoints500) {
        SendMark(kMarkCondition12_500);
      }
    } else if (next_trial_.win_probability_pct == kHotButtonWinPercent50) {
      if (next_trial_.hard_points == kHotButtonPoints300) {
        SendMark(kMarkCondition50_300);
      } else if (next_trial_.hard_points == kHotButtonPoints500) {
        SendMark(kMarkCondition50_500);
      }
    } else if (next_trial_.win_probability_pct == kHotButtonWinPercent88) {
      if (next_trial_.hard_points == kHotButtonPoints300) {
        SendMark(kMarkCondition88_300);
      } else if (next_trial_.hard_points == kHotButtonPoints500) {
        SendMark(kMarkCondition88_500);
      }
    }
  }

  void KeyPressed(SDL_Scancode scode) override {
    if (scode == next_scode_easy_) {
      engine_->SetEasyTrial(true);
      SendMark(kMarkTaskEasy);
      SwitchToScreen(0);
    } else if (scode == next_scode_hard_) {
      engine_->SetEasyTrial(false);
      SendMark(kMarkTaskHard);
      SwitchToScreen(0);
    }
  }

 private:
  std::shared_ptr<TextureManager> texture_manager_;
  std::shared_ptr<HotButtonEngine> engine_;

  HotButtonTrial next_trial_;
  SDL_Scancode next_scode_easy_;
  SDL_Scancode next_scode_hard_;

  SDL_Texture *dial_[kConditionNumDialImages];

  std::string next_win_percentage_;
  std::string left_task_;
  std::string right_task_;
  std::string left_task_points_;
  std::string right_task_points_;

  SDL_Point win_percentage_location_;
  SDL_Point left_task_location_;
  SDL_Point right_task_location_;
  SDL_Point left_task_points_location_;
  SDL_Point right_task_points_location_;
};

// This screen runs the button pressing game.
class TaskScreen : public Screen {
 public:
  TaskScreen(std::shared_ptr<TextureManager> texture_manager,
             std::shared_ptr<HotButtonEngine> engine)
      : texture_manager_(texture_manager), engine_(engine) {
    for (int i = 0; i < kNumDialImages; i++) {
      char filename[16];
      std::snprintf(filename, sizeof(filename), "dial%02d.svg", i);
      dial_[i] = texture_manager_->LoadImage(GetResourceDir() + filename);
    }

    int dial_width;
    int dial_height;
    SDL_QueryTexture(dial_[0], nullptr, nullptr, &dial_width, &dial_height);

    int display_width = GetDisplayWidthPx();
    int display_height = GetDisplayHeightPx();

    dial_rect_.w = dial_width;
    dial_rect_.h = dial_height;
    dial_rect_.y = (display_height - dial_height) / 2;

    progress_rect_.w = 300;
    progress_rect_.h = 600;
    progress_rect_.y = (display_height - progress_rect_.h) / 2;

    int total_width = dial_rect_.w + progress_rect_.w + 300;
    dial_rect_.x = (display_width - total_width) / 2;
    progress_rect_.x = dial_rect_.x + dial_width + 300;

    progress_filled_rect_ = progress_rect_;

    countdown_location_1_ = CenterStringInRect("0", dial_rect_, kFontScale);
    countdown_location_2_ = CenterStringInRect("00", dial_rect_, kFontScale);
  }

  void IsActive() override {
    if (engine_->GetEasyTrial()) {
      if (engine_->GetLeftHandedness()) {
        next_scode_ = kScancodeLeft;
      } else {
        next_scode_ = kScancodeRight;
      }
    } else {
      if (engine_->GetLeftHandedness()) {
        next_scode_ = kScancodeRight;
      } else {
        next_scode_ = kScancodeLeft;
      }
    }
    int timeout_ms = engine_->GetTrialTimeout();
    timeout_s_ = timeout_ms / 1000;
    start_time_ = 0;
    num_keypresses_ = 0;
    progress_filled_rect_.h = 0;
    SwitchToScreen(0, timeout_ms);
  }

  void IsInactive() override {
    if (num_keypresses_ == engine_->GetTrialNumKeypresses()) {
      engine_->SucceedTrial();
    } else {
      engine_->FailTrial();
    }
  }

  void Render() override {
    int countdown = 0;
    if (start_time_ == 0) {
      start_time_ = SDL_GetTicks();
      countdown = timeout_s_;
    } else {
      countdown = timeout_s_ - ((SDL_GetTicks() - start_time_) / 1000);
    }
    if (countdown < 0) {
      countdown = 0;
    }
    if (countdown > timeout_s_) {
      countdown = timeout_s_;
    }
    if (countdown >= 10) {
      DrawString(countdown_location_2_.x, countdown_location_2_.y,
                 std::to_string(countdown), kFontScale);
    } else {
      DrawString(countdown_location_1_.x, countdown_location_1_.y,
                 std::to_string(countdown), kFontScale);
    }
    Blit(dial_[countdown], dial_rect_);

    FillRect(progress_rect_, kDimColor, GetBackgroundColor());
    FillRect(progress_filled_rect_, GetLineColor(), GetBackgroundColor());

    if (num_keypresses_ == engine_->GetTrialNumKeypresses()) {
      SwitchToScreen(0);
    }
  }

  void IsVisible() override { SendMark(kMarkTimerStart); }

  void KeyPressed(SDL_Scancode scode) override {
    if (scode == next_scode_) {
      SendMark(kMarkKeypress);
      num_keypresses_ += 1;
      if (num_keypresses_ > engine_->GetTrialNumKeypresses()) {
        num_keypresses_ = engine_->GetTrialNumKeypresses();
      }
      double h = static_cast<double>(progress_rect_.h) /
                 engine_->GetTrialNumKeypresses() * num_keypresses_;
      progress_filled_rect_.h = static_cast<int>(h);
      progress_filled_rect_.y =
          progress_rect_.y + progress_rect_.h - progress_filled_rect_.h;
    }
  }

 private:
  std::shared_ptr<TextureManager> texture_manager_;
  std::shared_ptr<HotButtonEngine> engine_;

  SDL_Texture *dial_[kNumDialImages];
  SDL_Rect dial_rect_;
  SDL_Point countdown_location_1_;
  SDL_Point countdown_location_2_;
  SDL_Rect progress_rect_;
  SDL_Rect progress_filled_rect_;

  int start_time_;
  int timeout_s_;
  int num_keypresses_;
  SDL_Scancode next_scode_;
};

const std::string kSuccessString = "Success!";
const std::string kFailureString = "Failure!";

// This screen shows if the task was performed successfully.
class ResultScreen : public Screen {
 public:
  ResultScreen(std::shared_ptr<HotButtonEngine> engine, int timeout)
      : engine_(engine), timeout_(timeout) {
    success_location_ = CenterString(kSuccessString, kFontScale);
    failure_location_ = CenterString(kFailureString, kFontScale);
  }

  void IsActive() override {
    if (engine_->GetLastTrialSuccess()) {
      SwitchToScreen(0, timeout_);
    } else {
      SwitchToScreen(1, timeout_);
    }
  }

  void Render() override {
    if (engine_->GetLastTrialSuccess()) {
      DrawString(success_location_.x, success_location_.y, kSuccessString,
                 kFontScale);
    } else {
      DrawString(failure_location_.x, failure_location_.y, kFailureString,
                 kFontScale);
    }
  }

  void IsVisible() override {
    if (engine_->GetLastTrialSuccess()) {
      SendMark(kMarkTrialSuccess);
    } else {
      SendMark(kMarkTrialFailure);
    }
  }

 private:
  std::shared_ptr<HotButtonEngine> engine_;
  int timeout_;
  SDL_Point success_location_;
  SDL_Point failure_location_;
};

// This screen displays a image of dice.
class DiceScreen : public Screen {
 public:
  DiceScreen(int delay) : delay_(delay) {
    dice_ = LoadImage(GetResourceDir() + "dice.svg");
  };

  virtual ~DiceScreen() { SDL_DestroyTexture(dice_); }

  void IsActive() override { SwitchToScreen(0, delay_); }

  void Render() override { Blit(dice_); }

  void IsVisible() override { SendMark(kMarkDice); }

 private:
  SDL_Texture *dice_;
  int delay_;
};

const std::string kYouWonString = "You won!";
const std::string kYouLostString = "You lost!";
const std::string kPointsFeedbackString = " points!";

// This screen shows how many points was won in the last trial.
class FeedbackScreen : public Screen {
 public:
  FeedbackScreen(std::shared_ptr<HotButtonEngine> engine, int delay)
      : engine_(engine), delay_(delay) {}

  void IsActive() override {
    auto &won_loss_string =
        (engine_->GetLastTrialPoints() > 0) ? kYouWonString : kYouLostString;
    points_string_ =
        std::to_string(engine_->GetLastTrialPoints()) + kPointsFeedbackString;
    won_loss_location_ = CenterString(won_loss_string, kFontScale);
    points_location_ = CenterString(points_string_, kFontScale);
    int font_height = GetFontHeight(kFontScale);
    won_loss_location_.y -= font_height / 2;
    points_location_.y += font_height / 2;

    SwitchToScreen(0, delay_);
  }

  void Render() override {
    DrawString(
        won_loss_location_.x, won_loss_location_.y,
        engine_->GetLastTrialPoints() > 0 ? kYouWonString : kYouLostString,
        kFontScale);
    DrawString(points_location_.x, points_location_.y, points_string_,
               kFontScale);
  }

  void IsVisible() override {
    switch (engine_->GetLastTrialPoints()) {
      case kHotButtonPoints0:
        SendMark(kMarkFeedback0);
        break;
      case kHotButtonPoints100:
        SendMark(kMarkFeedback100);
        break;
      case kHotButtonPoints300:
        SendMark(kMarkFeedback300);
        break;
      case kHotButtonPoints500:
        SendMark(kMarkFeedback500);
        break;
    }
  }

 private:
  std::shared_ptr<HotButtonEngine> engine_;
  int delay_;
  std::string points_string_;
  SDL_Point won_loss_location_;
  SDL_Point points_location_;
};

const std::string kPointSummaryString = "Point Summary";
const std::string kSummaryContinueString = "Press any key to continue";
const std::string kZeroPointsString = "0";

// This screen shows a summary of the maximum points possible (if the hard
// task was selected every time, and points are always earned) vs. the current
// points earned.
class SummaryScreen : public Screen {
 public:
  SummaryScreen(std::shared_ptr<HotButtonEngine> engine) : engine_(engine) {
    int font_height = GetFontHeight(kFontScale);

    SDL_Rect display_rect;
    display_rect.x = 0;
    display_rect.y = 0;
    display_rect.w = GetDisplayWidthPx();
    display_rect.h = GetDisplayHeightPx() / 2;

    title_location_ =
        CenterStringInRect(kPointSummaryString, display_rect, kFontScale);
    title_location_.y -= font_height / 2;

    subtitle_location_.y = title_location_.y + font_height;

    display_rect.h = GetDisplayHeightPx();
    progress_rect_ = CenterRectInRect(1200, 400, display_rect);
    progress_filled_rect_ = progress_rect_;

    zero_points_string_location_.x =
        progress_rect_.x - GetStringWidth(kZeroPointsString, kFontScale) - 100;
    zero_points_string_location_.y =
        progress_rect_.y + (progress_rect_.h - GetFontHeight(kFontScale)) / 2;

    display_rect.h /= 2;
    display_rect.y = display_rect.h;
    footer_location_ =
        CenterStringInRect(kSummaryContinueString, display_rect, kFontScale);
  }

  void IsActive() override {
    int max = engine_->GetPotentialTotalPoints();
    int points = engine_->GetTotalPoints();

    subtitle_string_ =
        std::to_string(points) + "/" + std::to_string(max) + " points";
    subtitle_location_.x =
        (GetDisplayWidthPx() - GetStringWidth(subtitle_string_, kFontScale)) /
        2;

    max_points_string_ = std::to_string(max);
    max_points_string_location_.x = progress_rect_.x + progress_rect_.w + 100;
    max_points_string_location_.y = zero_points_string_location_.y;

    progress_filled_rect_.w = points * progress_rect_.w / max;
  }

  void Render() override {
    DrawString(title_location_.x, title_location_.y, kPointSummaryString,
               kFontScale);
    DrawString(subtitle_location_.x, subtitle_location_.y, subtitle_string_,
               kFontScale);
    DrawString(zero_points_string_location_.x, zero_points_string_location_.y,
               kZeroPointsString, kFontScale);
    DrawString(max_points_string_location_.x, max_points_string_location_.y,
               max_points_string_, kFontScale);
    DrawString(footer_location_.x, footer_location_.y, kSummaryContinueString,
               kFontScale);
    FillRect(progress_rect_, kDimColor, GetBackgroundColor());
    FillRect(progress_filled_rect_, GetLineColor(), GetBackgroundColor());
  }

  void IsVisible() override { SendMark(kMarkSummary); }

  void KeyPressed(SDL_Scancode scode) override {
    int current_time = SDL_GetTicks();
    if (current_time - engine_->GetStartTime() > kTotalTimeMs) {
      SwitchToScreen(1);
    } else {
      SwitchToScreen(0);
    }
  }

 private:
  std::shared_ptr<HotButtonEngine> engine_;
  std::string subtitle_string_;
  std::string max_points_string_;
  SDL_Point title_location_;
  SDL_Point subtitle_location_;
  SDL_Point footer_location_;
  SDL_Point zero_points_string_location_;
  SDL_Point max_points_string_location_;
  SDL_Rect progress_rect_;
  SDL_Rect progress_filled_rect_;
};

}  // namespace

Screen *InitHotButton(Screen *main_screen, const Settings &settings) {
  std::shared_ptr<TextureManager> texture_manager(new TextureManager());
  std::shared_ptr<HotButtonEngine> engine(new HotButtonEngine());
  std::shared_ptr<HotButtonEngine> demo_engine(new HotButtonDemoEngine());

  // Version
  Screen *version = new VersionScreen();

  // Pretask
  Screen *handedness = new HandednessScreen(engine, demo_engine);
  Screen *instructions = new PretaskInstructionScreen(engine);
  Screen *pretask_ready = new TimeoutScreen(kReady, kReadyTimeMs);
  Screen *pretask = new PretaskScreen(texture_manager, engine);
  Screen *pretask_finish =
      new MarkScreen(kMarkBaselineEnd, kPretaskFinishTimeMs);

  // Instructions
  Screen *instructions1 =
      new InstructionScreen("Press any key to begin the game");
  Screen *instructions2 = new HotButtonHandedInstructionScreen(
      engine, kInstructionsScreen2, kNumInstructionsScreen2);
  Screen *instructions3 = new HotButtonInstructionScreen(
      kInstructionsScreen3, kNumInstructionsScreen3);
  Screen *demo1 = new ConditionScreen(texture_manager, demo_engine);
  Screen *demo2 = new ConditionScreen(texture_manager, demo_engine);
  Screen *demo3 = new ConditionScreen(texture_manager, demo_engine);
  Screen *instructions4 = new HotButtonInstructionScreen(
      kInstructionsScreen4, kNumInstructionsScreen4);
  Screen *instructions5 = new HotButtonInstructionScreen(
      kInstructionsScreen5, kNumInstructionsScreen5);
  Screen *instructions6 = new HotButtonInstructionScreen(
      kInstructionsScreen6, kNumInstructionsScreen6);

  // Main task
  Screen *preload = new PreloadScreen(engine);
  Screen *condition = new ConditionScreen(texture_manager, engine);
  Screen *ready = new TimeoutScreen(kReady, kReadyTimeMs);
  Screen *task = new TaskScreen(texture_manager, engine);
  Screen *task_finish = new MarkScreen(kMarkTimerStop);
  Screen *fixation =
      new FixationDotScreen(kMinFixationTimeMs, kMaxFixationTimeMs);
  Screen *result = new ResultScreen(engine, kResultTimeMs);
  Screen *dice = new DiceScreen(kDiceTimeMs);
  Screen *feedback = new FeedbackScreen(engine, kFeedbackTimeMs);
  Screen *summary = new SummaryScreen(engine);
  Screen *finish = new MarkScreen(kMarkTaskStartStop);

  version->AddSuccessor(handedness);

  handedness->AddSuccessor(instructions);
  instructions->AddSuccessor(pretask_ready);
  pretask_ready->AddSuccessor(pretask);
  pretask->AddSuccessor(pretask_finish);
  pretask_finish->AddSuccessor(instructions1);

  instructions1->AddSuccessor(instructions2);
  instructions2->AddSuccessor(instructions3);
  instructions3->AddSuccessor(demo1);
  demo1->AddSuccessor(demo2);
  demo2->AddSuccessor(demo3);
  demo3->AddSuccessor(instructions4);
  instructions4->AddSuccessor(instructions5);
  instructions5->AddSuccessor(instructions6);
  instructions6->AddSuccessor(preload);

  preload->AddSuccessor(condition);
  condition->AddSuccessor(ready);
  ready->AddSuccessor(task);
  task->AddSuccessor(task_finish);
  task_finish->AddSuccessor(fixation);
  fixation->AddSuccessor(result);
  result->AddSuccessor(dice);
  result->AddSuccessor(summary);
  dice->AddSuccessor(feedback);
  feedback->AddSuccessor(summary);
  summary->AddSuccessor(condition);
  summary->AddSuccessor(finish);
  finish->AddSuccessor(main_screen);

  return version;
}

}  // namespace stimulus
