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
#include "Doors.h"
#include "Image.h"
#include "Mark.h"
#include "Random.h"
#include "Screen.h"
#include "Shuffler.h"

namespace stimulus {
namespace {

const float kDefaultWinLossWidthCm = 1;  // cm
const float kDoorsWidthCm = 10;  // cm
const float kDoorsHeightCm = 7.5;  // cm
const int kTotalTrials = 60;
const int kMaxRunLength = 3;
const char *kWinLossWidthSetting = "win_loss_width_cm";

// This screen doesn't have any interaction, it just serves as a point to
// compute the next result and check if the task is over.
class LoopScreen : public Screen {
 public:
  LoopScreen(Shuffler<bool> *shuffler) : shuffler_(shuffler) {
  }

  void IsVisible() override {
    if (shuffler_->IsDone()) {
      shuffler_->ShuffleElements();
      SwitchToScreen(1);  // Back to selection screen
    } else {
      SwitchToScreen(0);
    }
  }

 private:
  Shuffler<bool> *shuffler_;
};

class ChooseDoorScreen : public Screen {
 public:
  ChooseDoorScreen() {
    door_ = LoadImage(GetResourceDir() + "door.bmp");
    int door_source_width_px;
    int door_source_height_px;
    SDL_QueryTexture(door_, nullptr, nullptr, &door_source_width_px,
        &door_source_height_px);

    int total_dest_width_px = HorzSizeToPixels(kDoorsWidthCm);
    int door_dest_height_px = VertSizeToPixels(kDoorsHeightCm);
    float door_scale = static_cast<float>(door_dest_height_px)
         / door_source_height_px;
    int door_dest_width_px = door_scale * door_source_width_px;

    door1_rect_.x = (GetDisplayWidthPx() - total_dest_width_px) / 2;
    door1_rect_.y = (GetDisplayHeightPx() - total_dest_width_px) / 2;
    door1_rect_.w = door_dest_width_px;
    door1_rect_.h = door_dest_height_px;

    door2_rect_.x = (GetDisplayWidthPx() + total_dest_width_px) / 2
        - door_dest_width_px;
    door2_rect_.y = door1_rect_.y;
    door2_rect_.w = door_dest_width_px;
    door2_rect_.h = door_dest_height_px;
  }

  void Render() override {
    Blit(door_, door1_rect_);
    Blit(door_, door2_rect_);
  }

  void IsVisible() override {
    SendMark(10, "Stimulus");
  }

  void MouseClicked(int button, int x, int y) override {
    if (button == SDL_BUTTON_LEFT) {
      SendMark(1, "Response");
      SwitchToScreen(0);
    } else if (button == SDL_BUTTON_RIGHT) {
      SendMark(3, "Response");
      SwitchToScreen(0);
    }
  }

 private:
  SDL_Texture *door_;
  SDL_Rect door1_rect_;
  SDL_Rect door2_rect_;
};

class DoorsResultScreen : public Screen {
 public:
  DoorsResultScreen(Shuffler<bool> *shuffler, int win_loss_width_cm)
      : shuffler_(shuffler) {
    win_ = LoadImage(GetResourceDir() + "win.bmp");
    lose_ = LoadImage(GetResourceDir() + "lose.bmp");
    win_dest_rect_ = ComputeRectForPhysicalWidth(win_, win_loss_width_cm);
    lose_dest_rect_ = ComputeRectForPhysicalWidth(lose_, win_loss_width_cm);
  }

  void IsActive() override {
    is_win_ = shuffler_->GetNextItem();
  }

  void Render() override {
    if (is_win_) {
      Blit(win_, win_dest_rect_);
    } else {
      Blit(lose_, lose_dest_rect_);
    }
  }

  void IsVisible() override {
    if (is_win_) {
      SendMark(7, "PositiveFeedback");
    } else {
      SendMark(9, "NegativeFeedback");
    }

    SwitchToScreen(0, 2000);
  }

 private:
  bool is_win_;
  Shuffler<bool> *shuffler_;
  SDL_Texture *win_;
  SDL_Texture *lose_;
  SDL_Rect win_dest_rect_;
  SDL_Rect lose_dest_rect_;
};

}  // namespace

Screen *InitDoors(Screen *main_screen, const Settings &settings) {
  Shuffler<bool> *shuffler = new Shuffler<bool>();
  std::vector<bool> win_array;
  for (int i = 0; i < kTotalTrials / 2; i++) {
    win_array.push_back(true);
  }

  shuffler->AddCategoryElements(win_array, kMaxRunLength);

  std::vector<bool> lose_array;
  for (int i = 0; i < kTotalTrials / 2; i++) {
    lose_array.push_back(false);
  }

  shuffler->AddCategoryElements(lose_array, kMaxRunLength);
  shuffler->ShuffleElements();

  Screen *loop = new LoopScreen(shuffler);
  Screen *instructions = new InstructionScreen(
      "Click to start next round.");
  Screen *fixation1 = new FixationScreen(1000, 1000);
  Screen *choose_door = new ChooseDoorScreen();
  Screen *fixation2 = new FixationScreen(1000, 1000);

  int win_loss_width_cm = kDefaultWinLossWidthCm;
  if (settings.HasKey(kWinLossWidthSetting)) {
    win_loss_width_cm = settings.GetIntValue(kWinLossWidthSetting);
  }

  Screen *doors_result = new DoorsResultScreen(shuffler, win_loss_width_cm);
  Screen *fixation3 = new FixationScreen(1500, 1500);

  loop->AddSuccessor(instructions);
  loop->AddSuccessor(main_screen);
  instructions->AddSuccessor(fixation1);
  fixation1->AddSuccessor(choose_door);
  choose_door->AddSuccessor(fixation2);
  fixation2->AddSuccessor(doors_result);
  doors_result->AddSuccessor(fixation3);
  doors_result->AddSuccessor(main_screen);
  fixation3->AddSuccessor(loop);

  return loop;
}

}  // namespace stimulus
