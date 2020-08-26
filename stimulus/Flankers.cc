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

#include "Flankers.h"
#include "CommonScreens.h"
#include "FlankersEngine.h"
#include "Image.h"
#include "Mark.h"
#include "Shuffler.h"
#include "Util.h"

#include <cassert>
#include <iostream>

namespace stimulus {
namespace {

// Trial counts
const int kTotalTrials = 400;
const int kNumTrialsPerStimuli = 100;
const int kNumTrialsBeforeFeedback = 40;
const int kMaxRun = 3;

// Thresholds for feedback
const int kLowErrorPercentThreshold = 15;
const int kHighErrorPercentThreshold = 25;

// Marks
const int kMarkLeftCongruent = 44;
const int kMarkLeftIncongruent = 46;
const int kMarkRightCongruent = 66;
const int kMarkRightIncongruent = 64;
const int kMarkOffset = 77;
const int kMarkLeftResponse = 4;
const int kMarkRightResponse = 6;
const int kMarkNoResponse = 5;
const int kMarkFeedbackErrorHigh = 1;
const int kMarkFeedbackGood = 2;
const int kMarkFeedbackErrorLow = 3;
const int kMarkTaskStartStop = 999;

// Fixation duration
const int kMinFixationTimeMs = 1200;
const int kMaxFixationTimeMs = 1400;

// Stimuli parameters
const int kStimuliDisplayTimeMs = 200;
const float kStimuliHeightCm = 1.0;

// Keys
const SDL_Scancode kScancodeLeft = SDL_SCANCODE_D;
const SDL_Scancode kScancodeRight = SDL_SCANCODE_K;

// Settings
const char *kTotalTrialsSetting = "flankers_total_trials";
const char *kNumTrialsPerStimuliSetting = "flankers_num_trials_per_stimuli";
const char *kNumTrialsBeforeFeedbackSetting =
    "flankers_num_trials_before_feedback";

FlankersStimulus StimuliList[] = {
    {
        /*.mark = */kMarkLeftCongruent,
        /*.expected_response = */kCharLeft,
        /*.texture = */nullptr,
        /*.resource_name = */"left_congruent.svg",
        /*.event = */"StimulusLeftCongruent",
    },
    {
        /*.mark = */kMarkLeftIncongruent,
        /*.expected_response = */kCharLeft,
        /*.texture = */nullptr,
        /*.resource_name = */"left_incongruent.svg",
        /*.event = */"StimulusLeftIncongruent",
    },
    {
        /*.mark = */kMarkRightCongruent,
        /*.expected_response = */kCharRight,
        /*.texture = */nullptr,
        /*.resource_name = */"right_congruent.svg",
        /*.event = */"StimulusRightCongruent",
    },
    {
        /*.mark = */kMarkRightIncongruent,
        /*.expected_response = */kCharRight,
        /*.texture = */nullptr,
        /*.resource_name = */"right_incongruent.svg",
        /*.event = */"StimulusRightIncongruent",
    },
};

char ScodeToChar(SDL_Scancode scode) {
  switch (scode) {
    case kScancodeLeft:
      return kCharLeft;
    case kScancodeRight:
      return kCharRight;
    default:
      return kCharNone;
  }
}

class FlankersInstructionScreen : public InstructionExamplesScreen {
 public:
  FlankersInstructionScreen(FlankersEngine *engine,
                            std::shared_ptr<SDL_Texture> left_texture,
                            std::shared_ptr<SDL_Texture> right_texture)
      : InstructionExamplesScreen("Which way does the middle arrow point?",
                                  left_texture, right_texture, kStimuliHeightCm,
                                  "Left: Press \"D\"", "Right: Press \"K\""),
        engine_(engine) {}

  void IsActive() override { engine_->Reset(); }

  void IsVisible() override { SendMark(kMarkTaskStartStop); }

 private:
  FlankersEngine *engine_;
};

class FlankersFixationScreen : public FixationDotScreen {
 public:
  FlankersFixationScreen(FlankersEngine *engine, const int num_total_trials,
                         const int num_trials_before_feedback, int min_delay,
                         int max_delay)
      : FixationDotScreen(min_delay, max_delay),
        engine_(engine),
        num_total_trials_(num_total_trials),
        num_trials_before_feedback_(num_trials_before_feedback) {}

  void IsActive() override {
    keypressed_ = false;

    int trial_count = engine_->GetTrialCount();
    assert(trial_count > 0);

    // finish the task has priority over feedback
    if (trial_count == num_total_trials_) {
      next_screen_ = 2;
    } else if ((trial_count % num_trials_before_feedback_) == 0) {
      next_screen_ = 1;
    } else {
      next_screen_ = 0;
    }
  }

  void IsInactive() override {
    if (!keypressed_) {
      engine_->RecordKeyPress(kCharNone);
      SendMark(kMarkNoResponse);
    }
  }

  void KeyPressed(SDL_Scancode scode) override {
    if (!keypressed_) {
      keypressed_ = true;
      char c = ScodeToChar(scode);
      if (c == kCharLeft) {
        SendMark(kMarkLeftResponse, "ResponseLeft");
      } else if (c == kCharRight) {
        SendMark(kMarkRightResponse, "ResponseRight");
      }
      engine_->RecordKeyPress(c);
    }
  }

 private:
  FlankersEngine *engine_;
  const int num_total_trials_;
  const int num_trials_before_feedback_;
  bool keypressed_;
};

const std::string kFeedbackGood = "Good job!";
const std::string kFeedbackErrorLow = "Try to respond a bit faster!";
const std::string kFeedbackErrorHigh = "Try to respond more accurately!";
const std::string kFeedbackContinue = "Press any key to continue when ready.";

class FlankersFeedbackScreen : public Screen {
 public:
  FlankersFeedbackScreen(FlankersEngine *engine, const int num_total_trials,
                         const int num_trials_before_feedback)
      : engine_(engine),
        num_total_trials_(num_total_trials),
        num_trials_before_feedback_(num_trials_before_feedback) {
    // 2 lines of text: feedback and instruction to continue
    feedback_good_location_ = CenterString(kFeedbackGood);
    feedback_good_location_.y -= GetFontHeight();
    feedback_error_low_location_ = CenterString(kFeedbackErrorLow);
    feedback_error_low_location_.y = feedback_good_location_.y;
    feedback_error_high_location_ = CenterString(kFeedbackErrorHigh);
    feedback_error_high_location_.y = feedback_good_location_.y;

    continue_location_ = CenterString(kFeedbackContinue);
    continue_location_.y += GetFontHeight();
  }

  void IsActive() override {
    next_screen_ = engine_->GetTrialCount() == num_total_trials_ ? 1 : 0;
    error_pct_ = engine_->GetErrorPercent(num_trials_before_feedback_);
  }

  void IsInvisible() override { SendMark(kMarkOffset); }

  void Render() override {
    if (error_pct_ < kLowErrorPercentThreshold) {
      DrawString(feedback_error_low_location_.x, feedback_error_low_location_.y,
                 kFeedbackErrorLow);
    } else if (error_pct_ > kHighErrorPercentThreshold) {
      DrawString(feedback_error_high_location_.x,
                 feedback_error_high_location_.y, kFeedbackErrorHigh);
    } else {
      DrawString(feedback_good_location_.x, feedback_good_location_.y,
                 kFeedbackGood);
    }
    DrawString(continue_location_.x, continue_location_.y, kFeedbackContinue);
  }

  void IsVisible() override {
    if (error_pct_ < kLowErrorPercentThreshold) {
      SendMark(kMarkFeedbackErrorLow);
    } else if (error_pct_ > kHighErrorPercentThreshold) {
      SendMark(kMarkFeedbackErrorHigh);
    } else {
      SendMark(kMarkFeedbackGood);
    }
  }

  void KeyPressed(SDL_Scancode) override { SwitchToScreen(next_screen_); }

  void MouseClicked(int, int, int) override { SwitchToScreen(next_screen_); }

 private:
  FlankersEngine *engine_;
  const int num_total_trials_;
  const int num_trials_before_feedback_;
  int next_screen_;
  int error_pct_;
  SDL_Point feedback_good_location_;
  SDL_Point feedback_error_low_location_;
  SDL_Point feedback_error_high_location_;
  SDL_Point continue_location_;
};

class FlankersTrialScreen : public Screen {
 public:
  FlankersTrialScreen(FlankersEngine *engine) : engine_(engine) {
    dest_rect_ = ComputeRectForPhysicalHeight(StimuliList[0].texture.get(),
                                              kStimuliHeightCm);
  }

  void IsActive() override {
    next_stimulus_ = engine_->GetNextTrial();
    assert(next_stimulus_->texture != nullptr);
    SwitchToScreen(0, kStimuliDisplayTimeMs);
  }

  void Render() override { Blit(next_stimulus_->texture.get(), dest_rect_); }

  void IsVisible() override {
    SendMark(next_stimulus_->mark, next_stimulus_->event);
  }

  void IsInvisible() override { SendMark(kMarkOffset); }

 private:
  FlankersEngine *engine_;
  const FlankersStimulus *next_stimulus_;
  SDL_Rect dest_rect_;
};

}  // namespace

Screen *InitFlankers(Screen *main_screen, const Settings &settings) {
  // Load textures for stimuli
  int num_stimuli = 0;
  for (auto &s : StimuliList) {
    if (s.texture == nullptr) {
      s.texture = std::shared_ptr<SDL_Texture>(
          LoadImage(GetResourceDir() + s.resource_name), SDL_TextureDeleter());
    }
    num_stimuli += 1;
  }

  // Validate settings
  int num_trials_per_stimuli = kNumTrialsPerStimuli;
  int num_total_trials = kTotalTrials;
  if (settings.HasKey(kNumTrialsPerStimuliSetting) &&
      settings.HasKey(kTotalTrialsSetting)) {
    num_trials_per_stimuli = settings.GetIntValue(kNumTrialsPerStimuliSetting);
    num_total_trials = settings.GetIntValue(kTotalTrialsSetting);
    if (num_trials_per_stimuli * num_stimuli != num_total_trials) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error",
                               "Invalid setting for number of trials!",
                               nullptr);
      exit(1);
    }
  }
  int num_trials_before_feedback = kNumTrialsBeforeFeedback;
  if (settings.HasKey(kNumTrialsBeforeFeedbackSetting)) {
    num_trials_before_feedback =
        settings.GetIntValue(kNumTrialsBeforeFeedbackSetting);
  }

  FlankersEngine *engine = new FlankersEngine(StimuliList, num_stimuli,
                                              num_trials_per_stimuli, kMaxRun);

  Screen *version = new VersionScreen();
  Screen *instructions = new FlankersInstructionScreen(
      engine, StimuliList[0].texture, StimuliList[3].texture);
  Screen *initial_fixation =
      new FixationDotScreen(kMinFixationTimeMs, kMaxFixationTimeMs);
  Screen *fixation = new FlankersFixationScreen(
      engine, num_total_trials, num_trials_before_feedback, kMinFixationTimeMs,
      kMaxFixationTimeMs);
  Screen *feedback = new FlankersFeedbackScreen(engine, num_total_trials,
                                                num_trials_before_feedback);
  Screen *trial = new FlankersTrialScreen(engine);
  Screen *finish = new MarkScreen(kMarkTaskStartStop);

  version->AddSuccessor(instructions);
  instructions->AddSuccessor(initial_fixation);
  initial_fixation->AddSuccessor(trial);
  trial->AddSuccessor(fixation);
  fixation->AddSuccessor(trial);
  fixation->AddSuccessor(feedback);
  fixation->AddSuccessor(finish);
  feedback->AddSuccessor(initial_fixation);
  feedback->AddSuccessor(finish);
  finish->AddSuccessor(main_screen);

  return version;
}

}  // namespace stimulus
