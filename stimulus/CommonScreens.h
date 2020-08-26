/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_COMMONSCREENS_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_COMMONSCREENS_H_

#include <cassert>
#include <cstdio>
#include <memory>

#include "Image.h"
#include "Mark.h"
#include "Platform.h"
#include "Random.h"
#include "Screen.h"
#include "Util.h"
#include "Version.h"

namespace stimulus {

// This is a blank screen used to send a mark with an optional delay before
// switching to the next screen.
class MarkScreen : public Screen {
 public:
  MarkScreen(int mark, int delay = 0) : mark_(mark), delay_(delay) {}

  void IsActive() override {
    if (delay_ > 0) {
      SwitchToScreen(0, delay_);
    }
  }

  void IsVisible() override {
    SendMark(mark_);
    if (delay_ == 0) {
      SwitchToScreen(0);
    }
  }

 private:
  int mark_;
  int delay_;
};

// This is a blank screen used to send out the software version mark.
class VersionScreen : public Screen {
 public:
  void IsActive() override {
    // Use both IsActive() and IsVisible() hooks to send the two
    // version marks, because there needs to be at least one sampling
    // period between two marks for the bioamp to pick it up.
    unsigned major, minor, patch;
    int res = std::sscanf(kVersion.c_str(), "%u.%u.%u", &major, &minor, &patch);
    assert(res == 3);
    int version_mark = (0x7F << 24) | ((major & 0xFF) << 16) |
                       ((minor & 0xFF) << 8) | (patch & 0xFF);
    SendMark(version_mark);
  }

  void IsVisible() override {
    unsigned revision;
    int res = std::sscanf(REVISION, "%x", &revision);
    assert(res == 1);
    int revision_mark = (0x6 << 28) | (revision & 0x0FFFFFFF);
    SendMark(revision_mark);

    SwitchToScreen(0);
  }
};

const float kDefaultCrossWidthCm = 0.7;

class FixationScreen : public Screen {
 public:
  FixationScreen(int min_delay, int max_delay, float cross_width_cm = kDefaultCrossWidthCm)
      : min_delay_(min_delay), max_delay_(max_delay) {
    cross_ = LoadImage(GetResourceDir() + "cross.bmp");
    dest_rect_ = ComputeRectForPhysicalWidth(cross_, cross_width_cm);
  }

  void Render() override { Blit(cross_, dest_rect_); }

  void IsVisible() override {
    SwitchToScreen(0, GenerateRandomInt(min_delay_, max_delay_));
  }

 private:
  SDL_Texture *cross_;
  SDL_Rect dest_rect_;
  int min_delay_;
  int max_delay_;
};

const float kFixationDotWidthCm = 0.15;

class FixationDotScreen : public Screen {
 public:
  FixationDotScreen(int min_delay, int max_delay)
      : next_screen_(0), min_delay_(min_delay), max_delay_(max_delay) {
    dot_ = LoadImage(GetResourceDir() + "dot.bmp");
    dest_rect_ = ComputeRectForPhysicalWidth(dot_, kFixationDotWidthCm);
  }

  void Render() override { Blit(dot_, dest_rect_); }

  void IsVisible() override {
    SwitchToScreen(next_screen_, GenerateRandomInt(min_delay_, max_delay_));
  }

 protected:
  int next_screen_;

 private:
  SDL_Texture *dot_;
  SDL_Rect dest_rect_;
  int min_delay_;
  int max_delay_;
};

class InstructionScreen : public Screen {
 public:
  InstructionScreen(std::string instructions) : instructions_(instructions) {
    instruction_location_.x =
        (GetDisplayWidthPx() - GetStringWidth(instructions_)) / 2;
    instruction_location_.y = (GetDisplayHeightPx() - GetFontHeight()) / 2;
  }

  void Render() override {
    DrawString(instruction_location_.x, instruction_location_.y, instructions_);
  }

  void KeyPressed(SDL_Scancode) override { SwitchToScreen(0); }

  void MouseClicked(int button, int x, int y) override { SwitchToScreen(0); }

 private:
  std::string instructions_;
  SDL_Point instruction_location_;
};

// instruction screen with two example images, text instructions and
// descriptions above each example.
class InstructionExamplesScreen : public Screen {
 public:
  InstructionExamplesScreen(
      std::string instructions,
      std::shared_ptr<SDL_Texture> left_texture = nullptr,
      std::shared_ptr<SDL_Texture> right_texture = nullptr,
      int texture_height_cm = 0, std::string left_instructions = "",
      std::string right_instructions = "")
      : instructions_(instructions),
        left_instructions_(left_instructions),
        right_instructions_(right_instructions),
        left_texture_(left_texture),
        right_texture_(right_texture) {
    int display_width_px = GetDisplayWidthPx();
    int display_height_px = GetDisplayHeightPx();

    instruction_location_.x =
        (display_width_px - GetStringWidth(instructions_)) / 2;
    instruction_location_.y = ((display_height_px / 2) - GetFontHeight()) / 2;

    left_example_rect_.w = (display_width_px - kExampleMarginPx * 4) / 2;
    left_example_rect_.h = display_height_px / 2 - kExampleMarginPx * 2;
    left_example_rect_.x = kExampleMarginPx;
    left_example_rect_.y = (display_height_px / 2);

    SDL_Rect rect =
        ComputeRectForPhysicalHeight(left_texture_.get(), texture_height_cm);
    left_dest_rect_ = CenterRectInRect(rect, left_example_rect_);

    left_instruction_location_.x =
        left_example_rect_.x +
        (left_example_rect_.w - GetStringWidth(left_instructions_)) / 2;
    left_instruction_location_.y = left_example_rect_.y - kExampleMarginPx;

    right_example_rect_.w = left_example_rect_.w;
    right_example_rect_.h = left_example_rect_.h;
    right_example_rect_.x = display_width_px / 2 + kExampleMarginPx;
    right_example_rect_.y = left_example_rect_.y;

    rect = ComputeRectForPhysicalHeight(left_texture_.get(), texture_height_cm);
    right_dest_rect_ = CenterRectInRect(rect, right_example_rect_);

    right_instruction_location_.x =
        right_example_rect_.x +
        (right_example_rect_.w - GetStringWidth(right_instructions_)) / 2;
    right_instruction_location_.y = left_instruction_location_.y;
  }

  void Render() override {
    DrawString(instruction_location_.x, instruction_location_.y, instructions_);

    DrawString(left_instruction_location_.x, left_instruction_location_.y,
               left_instructions_);
    DrawString(right_instruction_location_.x, right_instruction_location_.y,
               right_instructions_);

    DrawRect(left_example_rect_, GetLineColor(), GetBackgroundColor());
    DrawRect(right_example_rect_, GetLineColor(), GetBackgroundColor());

    if (left_texture_ != nullptr) {
      Blit(left_texture_.get(), left_dest_rect_);
    }
    if (right_texture_ != nullptr) {
      Blit(right_texture_.get(), right_dest_rect_);
    }
  }

  void KeyPressed(SDL_Scancode) override { SwitchToScreen(0); }

  void MouseClicked(int, int, int) override { SwitchToScreen(0); }

 protected:
  SDL_Rect GetLeftExampleRect() { return left_example_rect_; }
  SDL_Rect GetRightExampleRect() { return right_example_rect_; }

 private:
  const int kExampleMarginPx = 100;

  std::string instructions_;
  SDL_Point instruction_location_;

  std::string left_instructions_;
  SDL_Point left_instruction_location_;

  std::string right_instructions_;
  SDL_Point right_instruction_location_;

  SDL_Rect left_example_rect_;
  SDL_Rect right_example_rect_;

  std::shared_ptr<SDL_Texture> left_texture_;
  std::shared_ptr<SDL_Texture> right_texture_;
  SDL_Rect left_dest_rect_;
  SDL_Rect right_dest_rect_;
};

// A screen of multi-line text. Each item in the lines parameter represents
// a line of text. The text is centered on the screen.
// If a non-zero mark is provided, this mark will be sent when the screen
// becomes visible.
class MultiLineScreen : public Screen {
 public:
  MultiLineScreen(const std::vector<std::string> &lines, int mark = 0,
                  bool use_mouse = true, bool use_keyboard = false)
      : mark_(mark), use_mouse_(use_mouse), use_keyboard_(use_keyboard) {
    Layout(lines);
  }

  void IsVisible() override {
    if (mark_ != 0) {
      SendMark(mark_);
    }
  }

  void Render() override {
    for (auto &line : lines_) {
      DrawString(line.x, line.y, line.str);
    }
  }

  void KeyPressed(SDL_Scancode) override { if (use_keyboard_) { SwitchToScreen(0);} }

  void MouseClicked(int, int, int) override { if (use_mouse_) { SwitchToScreen(0); } }

 protected:
  MultiLineScreen() {}

  void Layout(const std::vector<std::string> &lines) {
    lines_.clear();
    // layout the text vertically, and line break long lines
    int display_width = GetDisplayWidthPx();
    int font_height = GetFontHeight();
    // fixed-width font so one font_width is fine
    int font_width = GetStringWidth("_");
    unsigned max_chars = (display_width - kTextLeft * 2) / font_width;
    int y = 0;
    for (auto &str : lines) {
      // blank line
      if (str == "") {
        y += font_height;
      } else {
        int nlines = LineBreak(lines_, str, max_chars, y, font_height);
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
    for (auto &line : lines_) {
      int remaining_chars = max_chars - line.str.size();
      if (remaining_chars > 0) {
        remaining_chars /= 2;
      } else {
        remaining_chars = 0;
      }
      line.x = kTextLeft + remaining_chars * font_width;
      line.y += yoffset;
    }
  }

 private:
  // margins
  static const int kTextTop = 200;
  static const int kTextLeft = 400;
  int mark_ = 0;
  bool use_mouse_ = true;
  bool use_keyboard_ = false;
  std::vector<RenderString> lines_;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_COMMONSCREENS_H_
