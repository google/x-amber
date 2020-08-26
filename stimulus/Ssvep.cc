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

#include "Ssvep.h"

#include <cassert>
#include <memory>

#include "CommonScreens.h"
#include "Image.h"
#include "Mark.h"
#include "Random.h"
#include "Shuffler.h"
#include "TextureManager.h"

namespace stimulus {
namespace {

// Counts
const int kTotalTrials = 100;
const int kNumCategories = 2;
const int kNumConditions = 4;
const int kNumTrialsPerCondition = 25;
const int kNumImagesPerCategory = 27;
const int kNumTrialsBeforeBreak = 10;
const int kMaxRun = 0;

// Timing
const int kImageTimeMs = 150;
const int kMinFixationTimeMs = 2500;
const int kMaxFixationTimeMs = 3500;

// Conditions
const char kConditionPleasant = 'p';
const char kConditionNeutral = 'n';
const char kConditionUnpleasant = 'u';

// Marks
const int kMarkTaskStartStop = 999;
const int kMarkBreak = 888;
const int kMarkNeutralBase = 2000;
const int kMarkPleasantBase = 1000;
const int kMarkUnpleasantBase = 3000;

// Images
const std::string kNeutralImageFolder = "/ssvep/neutral/";
const std::string kPleasantImageFolder = "/ssvep/pleasant/";
const std::string kUnpleasantImageFolder = "/ssvep/unpleasant/";
const std::string kNeutralImageSuffix = "_neutral.jpg";
const std::string kPleasantImageSuffix = "_pleasant.jpg";
const std::string kUnpleasantImageSuffix = "_unpleasant.jpg";
const int kNumNeutralImages = 27;
const int kNumPleasantImages = 27;
const int kNumUnpleasantImages = 27;
const float kImageWidthCm = 10;

struct Condition {
  int mark;
  char condition[3];
} ConditionList[] = {
    {
        /*.mark = */ 1,
        /*.condition = */ "np",
    },
    {
        /*.mark = */ 2,
        /*.condition = */ "nu",
    },
    {
        /*.mark = */ 3,
        /*.condition = */ "pn",
    },
    {
        /*.mark = */ 4,
        /*.condition = */ "un",
    },
};

struct Image {
  std::string path;
  int mark;
};

struct SsvepState {
  TextureManager texture_manager;
  Shuffler<int> shuffler;
  Shuffler<Image> neutral_shuffler;
  Shuffler<Image> pleasant_shuffler;
  Shuffler<Image> unpleasant_shuffler;
  int trial_count;
  char *next_condition;
  int next_condition_mark;
  bool next_condition_mark_sent;
  bool first_image_mark_sent;
  int num_images;
  SDL_Texture *images[kNumImagesPerCategory * kNumCategories];
  int image_marks[kNumImagesPerCategory * kNumCategories];
  SDL_Rect dest_rects[kNumImagesPerCategory * kNumCategories];
};

void BuildImageList(TextureManager &texture_manager, Shuffler<Image> &shuffler,
                    const std::string &image_folder,
                    const std::string &image_suffix, int image_mark_base,
                    int image_count) {
  std::vector<Image> vector;
  for (int i = 0; i < image_count; i++) {
    std::string path =
        GetResourceDir() + image_folder + std::to_string(i + 1) + image_suffix;
    Image image = {path, image_mark_base + i + 1};
    // Pre-load images
    texture_manager.LoadImage(path);
    vector.push_back(image);
  }
  shuffler.AddCategoryElements(vector, 0);
}

class SsvepFixationScreen : public FixationDotScreen {
 public:
  SsvepFixationScreen(std::shared_ptr<SsvepState> state, int min_delay,
                      int max_delay)
      : FixationDotScreen(min_delay, max_delay), state_(state) {}

  void IsActive() override {
    assert(!state_->shuffler.IsDone());

    state_->trial_count += 1;

    // Draw condition
    int next = state_->shuffler.GetNextItem();
    state_->next_condition = ConditionList[next].condition;
    state_->next_condition_mark = ConditionList[next].mark;
    state_->num_images = 0;
    state_->next_condition_mark_sent = false;
    state_->first_image_mark_sent = false;

    state_->neutral_shuffler.ShuffleElements();
    state_->pleasant_shuffler.ShuffleElements();
    state_->unpleasant_shuffler.ShuffleElements();

    // Pre-load images
    for (int i = 0; i < kNumCategories * kNumImagesPerCategory; i++) {
      char category = state_->next_condition[i / kNumImagesPerCategory];
      Shuffler<Image> *image_shuffler = nullptr;
      switch (category) {
        case kConditionNeutral:
          image_shuffler = &state_->neutral_shuffler;
          break;
        case kConditionPleasant:
          image_shuffler = &state_->pleasant_shuffler;
          break;
        case kConditionUnpleasant:
          image_shuffler = &state_->unpleasant_shuffler;
          break;
        default:
          // should never happen
          assert(false);
          break;
      }
      Image next_image = image_shuffler->GetNextItem();
      state_->images[i] = state_->texture_manager.LoadImage(next_image.path);
      state_->image_marks[i] = next_image.mark;
      state_->dest_rects[i] =
          ComputeRectForPhysicalWidth(state_->images[i], kImageWidthCm);
    }
  }

 private:
  std::shared_ptr<SsvepState> state_;
};

class SsvepTrialScreen : public Screen {
 public:
  SsvepTrialScreen(std::shared_ptr<SsvepState> state) : state_(state) {}

  void IsActive() override {
    state_->num_images += 1;
    if (state_->num_images == kNumCategories * kNumImagesPerCategory) {
      if (state_->trial_count == kTotalTrials) {
        SwitchToScreen(3, kImageTimeMs);
      } else if ((state_->trial_count % kNumTrialsBeforeBreak) == 0) {
        SwitchToScreen(2, kImageTimeMs);
      } else {
        SwitchToScreen(1, kImageTimeMs);
      }
    } else {
      SwitchToScreen(0, kImageTimeMs);
    }
  }

  void Render() override {
    int i = state_->num_images - 1;
    Blit(state_->images[i], state_->dest_rects[i]);
    if (state_->next_condition_mark_sent && !state_->first_image_mark_sent &&
        (SDL_GetTicks() - condition_mark_timestamp_) > 10) {
      SendMark(state_->image_marks[i]);
      state_->first_image_mark_sent = true;
    }
  }

  void IsVisible() override {
    // The condition mark is time locked to the onset of the first image
    // The image id marks are sent to the onset of the image, except for
    // the first image, where it is sent some time after the condition
    // mark.
    if (state_->next_condition_mark_sent) {
      SendMark(state_->image_marks[state_->num_images - 1]);
    } else {
      SendMark(state_->next_condition_mark);
      state_->next_condition_mark_sent = true;
      // Need to timestamp this because the bioamp only caches one mark
      // at a time, so we must ensure the 2nd mark is sent out after at
      // least one sample passed (10ms for safety)
      condition_mark_timestamp_ = SDL_GetTicks();
    }
  }

 private:
  std::shared_ptr<SsvepState> state_;
  int condition_mark_timestamp_;
};

class BreakScreen : public InstructionScreen {
 public:
  BreakScreen(std::string instructions) : InstructionScreen(instructions) {}

  void IsVisible() override { SendMark(kMarkBreak); }
};

}  // namespace

Screen *InitSsvep(Screen *main_screen, const Settings &settings) {
  std::shared_ptr<SsvepState> state(new SsvepState{TextureManager(),
                                                   Shuffler<int>(),
                                                   Shuffler<Image>(),
                                                   Shuffler<Image>(),
                                                   Shuffler<Image>(),
                                                   0,
                                                   nullptr,
                                                   0,
                                                   false,
                                                   false,
                                                   0,
                                                   {nullptr},
                                                   {0},
                                                   {{0, 0, 0, 0}}});

  for (int i = 0; i < kNumConditions; i++) {
    std::vector<int> t(kNumTrialsPerCondition);
    std::fill(t.begin(), t.end(), i);
    state->shuffler.AddCategoryElements(t, kMaxRun);
  }
  state->shuffler.ShuffleElements();

  BuildImageList(state->texture_manager, state->neutral_shuffler,
                 kNeutralImageFolder, kNeutralImageSuffix, kMarkNeutralBase,
                 kNumNeutralImages);
  BuildImageList(state->texture_manager, state->pleasant_shuffler,
                 kPleasantImageFolder, kPleasantImageSuffix, kMarkPleasantBase,
                 kNumPleasantImages);
  BuildImageList(state->texture_manager, state->unpleasant_shuffler,
                 kUnpleasantImageFolder, kUnpleasantImageSuffix,
                 kMarkUnpleasantBase, kNumUnpleasantImages);

  Screen *version = new VersionScreen();
  Screen *start = new MarkScreen(kMarkTaskStartStop);
  Screen *instructions = new InstructionScreen("View each image");
  Screen *fixation =
      new SsvepFixationScreen(state, kMinFixationTimeMs, kMaxFixationTimeMs);
  // Need 2 instances of the trial screen so we can switch between them.
  Screen *trial1 = new SsvepTrialScreen(state);
  Screen *trial2 = new SsvepTrialScreen(state);
  Screen *rest =
      new BreakScreen("Take a break... Press any key when ready to proceed");
  Screen *finish = new MarkScreen(kMarkTaskStartStop);

  version->AddSuccessor(start);
  start->AddSuccessor(instructions);
  instructions->AddSuccessor(fixation);
  fixation->AddSuccessor(trial1);
  rest->AddSuccessor(fixation);
  trial1->AddSuccessor(trial2);
  trial1->AddSuccessor(fixation);
  trial1->AddSuccessor(rest);
  trial1->AddSuccessor(finish);
  trial2->AddSuccessor(trial1);
  trial2->AddSuccessor(fixation);
  trial2->AddSuccessor(rest);
  trial2->AddSuccessor(finish);
  finish->AddSuccessor(main_screen);

  return version;
}

}  // namespace stimulus
