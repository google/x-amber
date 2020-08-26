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

#include <cmath>
#include "CommonScreens.h"
#include "EmotionalImages.h"
#include "Image.h"
#include "Mark.h"
#include "Screen.h"
#include "Shuffler.h"
#include "Util.h"

namespace stimulus {
namespace {

const int kImageWidthCm = 15;
const int kDefaultImageDisplayTimeMs = 600;
const int kMinPreimageFixationTimeMs = 1000;
const int kMaxPreimageFixationTimeMs = 2000;
const int kNumRatingDots = 5;
const float kRatingCircleDiameterCm = 1.5;
const float kRatingDotDiameterCm = 0.5;
const int kRatingDelayMs = 150;
const char *kImageDisplayTimeSetting = "image_display_time_ms";
const int kMaxRunLength = 3;

// marks
const int kMarkTaskStartStop = 999;
const int kMarkStimulusOffset = 777;

struct EmotionalImage {
  std::string path;
  int mark;
};

struct EmotionalImagesState {
  std::string current_path;
  std::string image_folder;
  SDL_Texture *current_image = nullptr;
  SDL_Rect dest_rect;
  int current_mark;
  Shuffler<EmotionalImage> *shuffler = nullptr;
};

void BuildImageList(Shuffler<EmotionalImage>*);

// This is just a blank screen. It loads the image, then switches
// to the fixation screen. This ensures there is no delay when
// the image is displayed.
class PreloaderScreen : public Screen {
 public:
  PreloaderScreen(EmotionalImagesState *state) : state_(state) {
  }

  void IsVisible() override {
    if (state_->shuffler == nullptr) {
      state_->shuffler = new Shuffler<EmotionalImage>();
      BuildImageList(state_->shuffler);
    }

    if (state_->shuffler->IsDone()) {
      state_->current_image = nullptr;
      delete state_->shuffler;
      state_->shuffler = nullptr;
      SwitchToScreen(1);  // Back to selection screen
      return;
    }

    EmotionalImage image = state_->shuffler->GetNextItem();

    if (state_->current_image != nullptr) {
      SDL_DestroyTexture(state_->current_image);
    }

    state_->current_image = LoadImage(state_->image_folder + image.path);
    if (state_->current_image == nullptr) {
      // Exit
      SwitchToScreen(1);
      return;
    }

    state_->current_path = image.path;
    state_->current_mark = image.mark;
    state_->dest_rect = ComputeRectForPhysicalWidth(state_->current_image,
        kImageWidthCm);
    SwitchToScreen(0);
  }

 private:
  EmotionalImagesState *state_;
};

class ImageScreen : public Screen {
 public:
  ImageScreen(EmotionalImagesState *state, int image_display_time_ms)
    : state_(state), image_display_time_ms_(image_display_time_ms) {
  }

  void IsInvisible() override { SendMark(kMarkStimulusOffset); }

  void Render() override {
    Blit(state_->current_image, state_->dest_rect);
  }

  SDL_Color GetBackgroundColor() override {
    return SDL_Color { 0x60, 0x60, 0x60, 0xff };
  }

  void IsVisible() override {
    SendMark(state_->current_mark, state_->current_path);
    SwitchToScreen(0, image_display_time_ms_);
  }

 private:
  EmotionalImagesState *state_;
  int image_display_time_ms_;
};

class RatingScreen : public Screen {
 public:
  RatingScreen(const std::string &instructions, const std::string &low_label,
      const std::string &high_label, int mark_base_id)
      : instructions_(instructions),
        low_label_(low_label),
        high_label_(high_label),
        mark_base_id_(mark_base_id) {
    SetCursorVisible(true);

    circle_width_px_ = HorzSizeToPixels(kRatingCircleDiameterCm);
    circle_height_px_ = VertSizeToPixels(kRatingCircleDiameterCm);
    dot_x_inset_px_ = circle_width_px_ - HorzSizeToPixels(kRatingDotDiameterCm);
    dot_y_inset_px_ = circle_height_px_ - VertSizeToPixels(kRatingDotDiameterCm);

    rating_circle_ = LoadImage(GetResourceDir() + "rating-circle.bmp");
    rating_dot_ = LoadImage(GetResourceDir() + "rating-dot.bmp");

    int bounding_box_inset = GetDisplayWidthPx() / 3;
    rating_bounding_box_.w = GetDisplayWidthPx() - bounding_box_inset * 2;
    rating_bounding_box_.x = bounding_box_inset;
    rating_bounding_box_.h = circle_height_px_;
    rating_bounding_box_.y = (GetDisplayHeightPx() - circle_height_px_) / 2;
    dot_spacing_ = (rating_bounding_box_.w - circle_width_px_) / (kNumRatingDots - 1);

    int connector_height = rating_bounding_box_.h / 4;
    connector_rect_.x = rating_bounding_box_.x + circle_width_px_ / 2;
    connector_rect_.y = rating_bounding_box_.y + (rating_bounding_box_.h - connector_height) / 2;
    connector_rect_.w = rating_bounding_box_.w - circle_width_px_;
    connector_rect_.h = connector_height;

    int instruction_width = GetStringWidth(instructions);
    instruction_location_.x = (GetDisplayWidthPx() - instruction_width) / 2;
    instruction_location_.y = (GetDisplayHeightPx() / 4)
        - (GetFontHeight() / 2);

    int low_label_width = GetStringWidth(low_label_);

    // Center label over dot
    low_label_location_.x = rating_bounding_box_.x + (circle_width_px_
        - low_label_width) / 2;
    low_label_location_.y = rating_bounding_box_.y - GetFontHeight() * 2;

    int high_label_width = GetStringWidth(high_label_);
    high_label_location_.x = rating_bounding_box_.x + rating_bounding_box_.w
        - (circle_width_px_ + high_label_width) / 2;
    high_label_location_.y = rating_bounding_box_.y - GetFontHeight() * 2;
  }

  void IsActive() override {
    checked_item_ = -1;
  }

  void Render() override {
    DrawString(low_label_location_.x, low_label_location_.y, low_label_);
    DrawString(high_label_location_.x, high_label_location_.y, high_label_);
    DrawString(instruction_location_.x, instruction_location_.y, instructions_);
    SDL_SetRenderDrawColor(GetRenderer(), 255, 255, 255, 255);
    SDL_RenderFillRect(GetRenderer(), &connector_rect_);
    SDL_Rect rating_circle_rect { rating_bounding_box_.x, rating_bounding_box_.y, circle_width_px_, circle_height_px_ };
    for (int i = 0; i < kNumRatingDots; i++) {
      Blit(rating_circle_, rating_circle_rect);
      if (checked_item_ == i) {
        // Fill in this dot
        SDL_Rect dot_rect = stimulus::InsetRect(rating_circle_rect, dot_x_inset_px_, dot_y_inset_px_);
        Blit(rating_dot_, dot_rect);
      }

      rating_circle_rect.x += dot_spacing_;
    }

    SDL_SetRenderDrawColor(GetRenderer(), 96, 96, 96, 255);
  }

  void MouseClicked(int button, int x, int y) override {
    int dot_y = rating_bounding_box_.y + rating_bounding_box_.h / 2;
    float circle_r2_px = pow(circle_width_px_ / 2, 2);
    for (int i = 0; i < kNumRatingDots; i++) {
      int dot_x = rating_bounding_box_.x + circle_width_px_ / 2 + dot_spacing_ * i;
      float distance2 = pow(x - dot_x, 2) + pow(y - dot_y, 2);
      if (distance2 <= circle_r2_px) {
        checked_item_ = i;
        SendMark(mark_base_id_ + i, "Response");
        SwitchToScreen(0, kRatingDelayMs);
        break;
      }
    }
  }

 private:
  std::string instructions_;
  std::string low_label_;
  std::string high_label_;
  int dot_spacing_;
  SDL_Point low_label_location_;
  SDL_Point high_label_location_;
  int mark_base_id_;
  SDL_Texture *rating_dot_;
  SDL_Texture *rating_circle_;
  SDL_Rect rating_bounding_box_;
  int circle_width_px_;
  int circle_height_px_;
  SDL_Point instruction_location_;
  SDL_Rect connector_rect_;
  int checked_item_ = -1;
  int dot_x_inset_px_;
  int dot_y_inset_px_;
};

void BuildImageList(std::vector<EmotionalImage> *out_vec,
    const std::string &folder_name, const std::string &suffix, int count,
    int mark_start_id) {
  for (int i = 0; i < count; i++) {
    std::string path = folder_name + "/" + std::to_string(i + 1) + "_" + suffix
        + ".jpg";
    EmotionalImage image = { path,  mark_start_id + i };
    out_vec->push_back(image);
  }
}

void BuildImageList(Shuffler<EmotionalImage> *shuffler) {
  std::vector<EmotionalImage> pleasantImages;
  BuildImageList(&pleasantImages, "pleasant_families_groups", "posgrp", 20, 1);
  BuildImageList(&pleasantImages, "pleasant_babies", "posbaby", 20, 21);
  BuildImageList(&pleasantImages, "pleasant_animals", "posaml", 20, 41);
  shuffler->AddCategoryElements(pleasantImages, kMaxRunLength);

  std::vector<EmotionalImage> neutralImages;
  BuildImageList(&neutralImages, "neutral_people", "neutppl", 40, 61);
  BuildImageList(&neutralImages, "neutral_animals", "neutaml", 20, 101);
  shuffler->AddCategoryElements(neutralImages, kMaxRunLength);

  std::vector<EmotionalImage> unpleasantImages;
  BuildImageList(&unpleasantImages, "unpleasant_sadness", "negsad", 20, 121);
  BuildImageList(&unpleasantImages, "unpleasant_disgust", "negdis", 20, 141);
  BuildImageList(&unpleasantImages, "unpleasant_animals", "negaml", 20, 161);
  shuffler->AddCategoryElements(unpleasantImages, kMaxRunLength);
  shuffler->ShuffleElements();
}

}  // namespace

Screen *InitEmotionalImages(Screen *main_screen, const Settings &settings) {
  EmotionalImagesState *state = new EmotionalImagesState();
  state->image_folder = stimulus::GetResourceDir() + "/emotional_images/";

  Screen *version = new VersionScreen();
  Screen *start_screen = new InstructionScreen(
      "Click to begin.");
  Screen *start_mark_screen = new MarkScreen(kMarkTaskStartStop);
  Screen *preloader = new PreloaderScreen(state);
  Screen *fixation1 = new FixationScreen(kMinPreimageFixationTimeMs,
      kMaxPreimageFixationTimeMs);
  Screen *finish = new MarkScreen(kMarkTaskStartStop);

  int image_display_time_ms = kDefaultImageDisplayTimeMs;
  if (settings.HasKey(kImageDisplayTimeSetting)) {
    image_display_time_ms = settings.GetIntValue(kImageDisplayTimeSetting);
  }

  Screen *image = new ImageScreen(state, image_display_time_ms);

#if ENABLE_RATINGS_SCREEN
    Screen *rating1 = new RatingScreen(
        "How excited did this image make you feel?", "Calm", "Tense", 181);
    Screen *rating2 = new RatingScreen(
        "What was your emotion when you saw this image", "Unhappy", "Happy",191);
    image->AddSuccessor(rating1);
    rating1->AddSuccessor(rating2);
    rating2->AddSuccessor(preloader);
#endif

    image->AddSuccessor(preloader);

    version->AddSuccessor(start_screen);
    start_screen->AddSuccessor(start_mark_screen);
    start_mark_screen->AddSuccessor(preloader);
    preloader->AddSuccessor(fixation1);
    preloader->AddSuccessor(finish);
    fixation1->AddSuccessor(image);
    finish->AddSuccessor(main_screen);

    return version;
}

}  // namespace stimulus
