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

#include "Calibration.h"
#include "CommonScreens.h"
#include "Image.h"
#include "Mark.h"
#include "Random.h"
#include "Screen.h"
#include "Shuffler.h"
#include "Util.h"

#include <cassert>

namespace stimulus {
namespace {

// Counts
const int kTotalTrials = 400;
const int kNumTrialsBeforeRest = 40;
const int kNumRareStimuli = 80;
const int kNumStandardStimuli = kTotalTrials - kNumRareStimuli;
const int kMaxRunRare = 2;
const int kMaxRunStandard = 0;

// Marks
const int kMarkRare = 2;
const int kMarkStandard = 1;
const int kMarkOffset = 777;
const int kMarkStartStop = 999;
const int kMarkRest = 888;

// Timing
const int kStimuliDisplayTimeMs = 100;
const int kMinFixationTimeMs = 700;
const int kMaxFixationTimeMs = 800;
const int kRestFixationTimeMs = 2000;

// Dimensions
const float kStimuliHeightCm = 1.0;

class CalibrationResultScreen : public Screen {
 public:
  CalibrationResultScreen(std::shared_ptr<SDL_Texture> rare,
                          std::shared_ptr<SDL_Texture> standard)
      : rare_(rare), standard_(standard) {
    dest_rect_ = ComputeRectForPhysicalHeight(rare_.get(), kStimuliHeightCm);

    // use Shuffler class to ensure we generate exactlythe number of
    // required rare and standard stimuli, and that no rare stimuli is
    // presented twice in a row.
    std::vector<int> r(kNumRareStimuli);
    std::fill(r.begin(), r.end(), kMarkRare);
    shuffler_.AddCategoryElements(r, kMaxRunRare);

    std::vector<int> s(kNumStandardStimuli);
    std::fill(s.begin(), s.end(), kMarkStandard);
    shuffler_.AddCategoryElements(s, kMaxRunStandard);

    shuffler_.ShuffleElements();
  }

  void IsActive() override {
    assert(!shuffler_.IsDone());
    next_is_rare_ = shuffler_.GetNextItem() == kMarkRare;
    if (trial_count_ == 0) {
      SendMark(kMarkStartStop, "TaskStartStop");
    }

    trial_count_ += 1;
    if (trial_count_ == kTotalTrials) {
      SwitchToScreen(1, kStimuliDisplayTimeMs);
    } else if (trial_count_ % kNumTrialsBeforeRest == 0) {
      SwitchToScreen(2, kStimuliDisplayTimeMs);
    } else {
      SwitchToScreen(0, kStimuliDisplayTimeMs);
    }
  }

  void IsInvisible() override {
    SendMark(kMarkOffset, "StimulusOffset");

    if (trial_count_ == kTotalTrials) {
      SendMark(kMarkStartStop, "TaskStartStop");
      assert(standard_count_ == kNumStandardStimuli);
      assert(rare_count_ == kNumRareStimuli);
      trial_count_ = 0;
      standard_count_ = 0;
      rare_count_ = 0;
      // reshuffle elements for next time
      shuffler_.ShuffleElements();
    }
  }

  void Render() override {
    Blit(next_is_rare_ ? rare_.get() : standard_.get(), dest_rect_);
  }

  void IsVisible() override {
    if (next_is_rare_) {
      SendMark(kMarkRare, "StimulusRare");
      rare_count_ += 1;
    } else {
      SendMark(kMarkStandard, "StimulusStandard");
      standard_count_ += 1;
    }
  }

 private:
  std::shared_ptr<SDL_Texture> rare_;
  std::shared_ptr<SDL_Texture> standard_;
  SDL_Rect dest_rect_;
  int trial_count_ = 0;
  int standard_count_ = 0;
  int rare_count_ = 0;
  Shuffler<int> shuffler_;
  bool next_is_rare_;
};

class RestScreen : public InstructionScreen {
public:
  RestScreen(std::string instructions) : InstructionScreen(instructions) {}

  void IsVisible() override {
    SendMark(kMarkRest, "TaskRest");
  }
};

}  // namespace

Screen *InitCalibration(Screen *main_screen) {
  std::shared_ptr<SDL_Texture> rare(LoadImage(GetResourceDir() + "square.svg"),
                                    SDL_TextureDeleter());
  std::shared_ptr<SDL_Texture> standard(LoadImage(GetResourceDir() + "o.svg"),
                                        SDL_TextureDeleter());

  Screen *version = new VersionScreen();
  Screen *instructions = new InstructionExamplesScreen(
      "Silently count the number of squares that appear.", rare, standard,
      kStimuliHeightCm);
  Screen *fixation =
      new FixationDotScreen(kMinFixationTimeMs, kMaxFixationTimeMs);
  Screen *rest =
      new RestScreen("Take a break... click when ready to proceed");
  Screen *restFixation =
      new FixationDotScreen(kRestFixationTimeMs, kRestFixationTimeMs);
  Screen *calibrationResult = new CalibrationResultScreen(rare, standard);

  version->AddSuccessor(instructions);
  instructions->AddSuccessor(fixation);
  fixation->AddSuccessor(calibrationResult);
  rest->AddSuccessor(restFixation);
  restFixation->AddSuccessor(calibrationResult);
  calibrationResult->AddSuccessor(fixation);
  calibrationResult->AddSuccessor(main_screen);
  calibrationResult->AddSuccessor(rest);

  return version;
}

}  // namespace stimulus
