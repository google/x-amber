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

#include "Sret.h"
#include "CommonScreens.h"
#include "Image.h"
#include "Mark.h"
#include "Shuffler.h"
#include "SretWordList.h"
#include "Util.h"

#include <cassert>

namespace stimulus {
namespace {

// Trial counts
const int kTotalTrials = 80;
const int kMaxRun = 1;

// Durations
const int kMinFixationTimeMs = 1500;
const int kMaxFixationTimeMs = 1700;
const int kResponseFixationMs = 1800;
const int kWordTimeMs = 400;
const int kResponseTimeoutMs = 5000;

// Marks
const int kMarkNo = 100;
const int kMarkYes = 200;
const int kMarkNoResponse = 255;
const int kMarkStartStop = 999;
const int kMarkOffset = 777;

// Keys
const SDL_Scancode kScancodeNo = SDL_SCANCODE_S;
const SDL_Scancode kScancodeYes = SDL_SCANCODE_L;

struct SretWord {
  std::string word;
  int mark;
};

// State
bool Done = false;

class SretInstructionScreen : public InstructionExamplesScreen {
 public:
  SretInstructionScreen() : InstructionExamplesScreen("Rate each word") {
    left_location_ =
        CenterStringInRect(kLeftStr, GetLeftExampleRect(), kLeftFontScale);
    right1_location_ = CenterStringInRect(kRightStrLine1, GetRightExampleRect(),
                                          kRightFontScale);
    right2_location_ = CenterStringInRect(kRightStrLine2, GetRightExampleRect(),
                                          kRightFontScale);

    int font_height = GetFontHeight(kRightFontScale);
    right1_location_.y -= font_height;
    right2_location_.y += font_height;

    Done = false;
  }

  void Render() override {
    InstructionExamplesScreen::Render();
    DrawString(left_location_.x, left_location_.y, kLeftStr, kLeftFontScale);
    DrawString(right1_location_.x, right1_location_.y, kRightStrLine1,
               kRightFontScale);
    DrawString(right2_location_.x, right2_location_.y, kRightStrLine2,
               kRightFontScale);
  }

  void IsVisible() override {
    SendMark(kMarkStartStop, "TaskStartStop");
  }

 private:
  const float kLeftFontScale = 1.6;
  const float kRightFontScale = 1.0;
  const std::string kLeftStr = "excited";
  const std::string kRightStrLine1 = "Does this word describe you?";
  const std::string kRightStrLine2 = "No \"S\"    Yes \"L\"";

  SDL_Point left_location_;
  SDL_Point right1_location_;
  SDL_Point right2_location_;
};

class SretWordScreen : public Screen {
 public:
  SretWordScreen(const std::vector<std::string> negative_words,
                 const std::vector<std::string> positive_words)
      : trial_count_(0) {
    // marks are positive words followed by negative words, 1-based
    int count = positive_words.size();
    std::vector<SretWord> positive;
    for (int i = 0; i < count; i++) {
      SretWord word = {positive_words.at(i), i + 1};
      positive.push_back(word);
    }
    shuffler_.AddCategoryElements(positive, kMaxRun);

    std::vector<SretWord> negative;
    for (int i = 0; i < static_cast<int>(negative_words.size()); i++) {
      SretWord word = {negative_words.at(i), i + 1 + count};
      negative.push_back(word);
    }
    shuffler_.AddCategoryElements(negative, kMaxRun);

    shuffler_.ShuffleElements();
  }

  void IsActive() override {
    trial_count_ += 1;
    assert(!shuffler_.IsDone());

    SretWord word = shuffler_.GetNextItem();
    next_word_ = word.word;
    next_mark_ = word.mark;
    word_location_ = CenterString(next_word_, 1.0);

    if (trial_count_ == kTotalTrials) {
      Done = true;
      shuffler_.ShuffleElements();
      trial_count_ = 0;
    }
    SwitchToScreen(0, kWordTimeMs);
  }

  void IsInvisible() override {
    SendMark(kMarkOffset, "WordOffset");
  }

  void Render() override {
    DrawString(word_location_.x, word_location_.y, next_word_, 1.0);
  }

  void IsVisible() override {
    SendMark(next_mark_, next_word_);
  }

 private:
  int trial_count_;
  std::string next_word_;
  int next_mark_;
  SDL_Point word_location_;
  Shuffler<SretWord> shuffler_;
};

class SretResponseScreen : public Screen {
 public:
  SretResponseScreen() {
    line1_location_ = CenterString(kLine1);
    line1_location_.y -= GetFontHeight();
    line2_location_ = CenterString(kLine2);
    line2_location_.y += GetFontHeight();
  }

  void IsActive() override {
    keypressed_ = false;
    if (Done) {
      SwitchToScreen(1, kResponseTimeoutMs);
    } else {
      SwitchToScreen(0, kResponseTimeoutMs);
    }
  }

  void IsInactive() override {
    if (!keypressed_) {
      SendMark(kMarkNoResponse, "ResponseNone");
    }
    if (Done) {
      SendMark(kMarkStartStop, "TaskStartStop");
    }
  }

  void Render() override {
    DrawString(line1_location_.x, line1_location_.y, kLine1);
    DrawString(line2_location_.x, line2_location_.y, kLine2);
  }

  void KeyPressed(SDL_Scancode scode) override {
    switch (scode) {
      case kScancodeYes:
        keypressed_ = true;
        SendMark(kMarkYes, "ResponseYes");
        break;
      case kScancodeNo:
        keypressed_ = true;
        SendMark(kMarkNo, "ResponseNo");
        break;
      default:
        break;
    }
    if (keypressed_) {
      if (Done) {
        SwitchToScreen(1);
      } else {
        SwitchToScreen(0);
      }
    }
  }

 private:
  const std::string kLine1 = "Does this word describe you?";
  const std::string kLine2 = "No \"S\"    Yes \"L\"";
  SDL_Point line1_location_;
  SDL_Point line2_location_;
  bool keypressed_;
};

}  // namespace

Screen *InitSret(Screen *main_screen, const Settings &settings) {
  Screen *version = new VersionScreen();
  Screen *instructions = new SretInstructionScreen();
  Screen *fixation =
      new FixationDotScreen(kMinFixationTimeMs, kMaxFixationTimeMs);
  Screen *word = new SretWordScreen(SretNegativeWords, SretPositiveWords);
  Screen *response_fixation =
      new FixationDotScreen(kResponseFixationMs, kResponseFixationMs);
  Screen *response = new SretResponseScreen();

  version->AddSuccessor(instructions);
  instructions->AddSuccessor(fixation);
  fixation->AddSuccessor(word);
  word->AddSuccessor(response_fixation);
  response_fixation->AddSuccessor(response);
  response->AddSuccessor(fixation);
  response->AddSuccessor(main_screen);

  return version;
}

}  // namespace stimulus
