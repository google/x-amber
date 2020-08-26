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

#include "Screen.h"

#include <cassert>

#include "Image.h"
#include "Platform.h"
#include "Util.h"

namespace stimulus {
namespace {
void ReportSdlError(const std::string &call) {
  Screen::FatalError(call + " error: " + SDL_GetError());
}

const int kGlyphWidth = 60;
const char kLowestGlyph = '!';
const char kHighestGlyph = '~';

}  // namespace

Screen *Screen::previous_screen_;
Screen *Screen::current_screen_;
Screen *Screen::next_screen_;
SDL_Renderer *Screen::renderer_;
int Screen::next_screen_presentation_time_;
int Screen::presentation_countdown_;
int Screen::display_width_px_;
int Screen::display_height_px_;
float Screen::pixel_ratio_;
SDL_Window *Screen::window_;
float Screen::horz_pixels_per_cm_;
float Screen::vert_pixels_per_cm_;
SDL_Texture *Screen::font_atlas_;
float Screen::font_scale_;
int Screen::glyph_height_;
bool Screen::enable_sdl_error_dialog_;

SDL_Renderer *Screen::GetRenderer() { return renderer_; }

void Screen::FatalError(const std::string &error) {
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", error.c_str(),
                           nullptr);
  exit(1);
}

void Screen::SwitchToScreen(int successor_num, int delay_ms) {
  assert((unsigned int)successor_num < successors_.size());
  next_screen_ = successors_[successor_num];
  next_screen_presentation_time_ = SDL_GetTicks() + delay_ms - 1;
}

void Screen::Blit(SDL_Texture *texture) {
  SDL_Rect dest;
  SDL_QueryTexture(texture, nullptr, nullptr, &dest.w, &dest.h);
  dest.x = (display_width_px_ - dest.w) / 2;
  dest.y = (display_height_px_ - dest.h) / 2;
  Blit(texture, dest);
}

void Screen::Blit(SDL_Texture *texture, int x, int y) {
  SDL_Rect dest;
  SDL_QueryTexture(texture, nullptr, nullptr, &dest.w, &dest.h);
  dest.x = x;
  dest.y = y;
  Blit(texture, dest);
}

void Screen::Blit(SDL_Texture *texture, const SDL_Rect &dest_rect) {
  SDL_RenderCopy(renderer_, texture, nullptr, &dest_rect);
}

void Screen::DrawRect(const SDL_Rect &rect, const SDL_Color &line_color,
                      const SDL_Color &bg_color) {
  SDL_SetRenderDrawColor(renderer_, line_color.r, line_color.g, line_color.b,
                         line_color.a);
  SDL_RenderDrawRect(renderer_, &rect);
  SDL_SetRenderDrawColor(renderer_, bg_color.r, bg_color.g, bg_color.b,
                         bg_color.a);
}

void Screen::FillRect(const SDL_Rect &rect, const SDL_Color &fill_color,
                      const SDL_Color &bg_color) {
  SDL_SetRenderDrawColor(renderer_, fill_color.r, fill_color.g, fill_color.b,
                         fill_color.a);
  SDL_RenderFillRect(renderer_, &rect);
  SDL_SetRenderDrawColor(renderer_, bg_color.r, bg_color.g, bg_color.b,
                         bg_color.a);
}

void Screen::DrawString(int left, int top, const std::string &str,
                        float font_scale) {
  SDL_Rect source_rect;
  source_rect.y = 0;
  source_rect.w = kGlyphWidth;
  source_rect.h = glyph_height_;

  SDL_Rect dest_rect;
  dest_rect.x = left;
  dest_rect.y = top;
  dest_rect.w = kGlyphWidth * font_scale_ * font_scale;
  dest_rect.h = glyph_height_ * font_scale_ * font_scale;

  for (auto ch : str) {
    if (ch >= kLowestGlyph && ch <= kHighestGlyph) {
      source_rect.x = (ch - kLowestGlyph) * kGlyphWidth;
      SDL_RenderCopy(renderer_, font_atlas_, &source_rect, &dest_rect);
    }

    dest_rect.x += kGlyphWidth * font_scale_ * font_scale;
  }
}

int Screen::GetStringWidth(const std::string &str, float font_scale) {
  return str.length() * kGlyphWidth * font_scale_ * font_scale;
}

int Screen::GetFontHeight(float font_scale) {
  return glyph_height_ * font_scale_ * font_scale;
}

SDL_Rect Screen::ComputeRectForPhysicalWidth(SDL_Texture *texture,
                                             float width_cm) {
  int texture_width;
  int texture_height;
  SDL_QueryTexture(texture, nullptr, nullptr, &texture_width, &texture_height);

  int pixel_width = HorzSizeToPixels(width_cm);
  float scale = static_cast<float>(pixel_width) / texture_width;
  int pixel_height = scale * texture_height;

  return CenterRect(pixel_width, pixel_height);
}

SDL_Rect Screen::ComputeRectForPhysicalHeight(SDL_Texture *texture,
                                              float height_cm) {
  int texture_width;
  int texture_height;
  SDL_QueryTexture(texture, nullptr, nullptr, &texture_width, &texture_height);

  int pixel_height = VertSizeToPixels(height_cm);
  float scale = static_cast<float>(pixel_height) / texture_height;
  int pixel_width = scale * texture_width;

  return CenterRect(pixel_width, pixel_height);
}

bool Screen::InitDisplay(float screen_width_cm, float screen_height_cm) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    ReportSdlError("SDL_GetCurrentDisplayMode");
    return false;
  }

  window_ = SDL_CreateWindow("Stimulus", 0, 0, 0, 0,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP |
                                 SDL_WINDOW_ALLOW_HIGHDPI);
  if (window_ == nullptr) {
    ReportSdlError("SDL_CreateWindow");
    SDL_Quit();
    return false;
  }

  renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer_ == nullptr) {
    ReportSdlError("SDL_CreateRenderer");
    SDL_Quit();
    return false;
  }

  if (SDL_GetRendererOutputSize(renderer_, &display_width_px_,
                                &display_height_px_) < 0) {
    ReportSdlError("SDL_CreateRenderer");
    SDL_Quit();
    return false;
  }

  // SDL_GetWindowSize returns the size of the window in screen coordinates.
  // Screen coordinates are used in mouse events.
  int window_width_px, window_height_px;
  SDL_GetWindowSize(window_, &window_width_px, &window_height_px);

  pixel_ratio_ = static_cast<float>(display_width_px_) /
                 static_cast<float>(window_width_px);

  // 6400 is chosen based on font atlas size
  font_scale_ = static_cast<float>(display_width_px_) / 6400.0;

  SDL_Log("Window size %dx%d\n", window_width_px, window_height_px);
  SDL_Log("Renderer size %dx%d\n", display_width_px_, display_height_px_);
  SDL_Log("Screen is %fx%f\n", screen_width_cm, screen_height_cm);
  SDL_Log("Pixel ratio is %f\n", pixel_ratio_);
  SDL_Log("Font scale is %f\n", font_scale_);

  horz_pixels_per_cm_ = display_width_px_ / screen_width_cm;
  vert_pixels_per_cm_ = display_height_px_ / screen_height_cm;

  font_atlas_ = LoadImage(GetResourceDir() + "font.svg");
  if (font_atlas_ == nullptr) {
    SDL_Quit();
    FatalError("Couldn't load font");
    return false;
  }

  int glyph_width;
  if (SDL_QueryTexture(font_atlas_, nullptr, nullptr, &glyph_width,
                       &glyph_height_) < 0) {
    SDL_Quit();
    FatalError("SDL_QueryTexture failed");
    return false;
  }

  SDL_Log("font %dx%d\n", glyph_width, glyph_height_);

  enable_sdl_error_dialog_ = true;
  return true;
}

void Screen::MainLoop(Screen *initial_screen) {
  next_screen_ = initial_screen;
  next_screen_presentation_time_ = SDL_GetTicks();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;

        case SDL_MOUSEBUTTONDOWN:
          if (current_screen_ != nullptr) {
            current_screen_->MouseClicked(event.button.button, event.button.x,
                                          event.button.y);
          }
          break;

        case SDL_KEYDOWN:
          if ((event.key.keysym.mod & KMOD_LCTRL) != 0 &&
              event.key.keysym.scancode == SDL_SCANCODE_C) {
            // CTRL-C will exit the application
            running = false;
          } else if (current_screen_ != nullptr) {
            // Ignore repeated keys
            if (event.key.repeat == 0) {
              current_screen_->KeyPressed(event.key.keysym.scancode);
            }
          }
          break;
      }
    }

    if (next_screen_ != current_screen_ &&
        WrappedGreaterEqual(SDL_GetTicks(), next_screen_presentation_time_)) {
      if (current_screen_) {
        current_screen_->IsInactive();
      }

      previous_screen_ = current_screen_;
      current_screen_ = next_screen_;
      SDL_ShowCursor(current_screen_->cursor_visible_ ? SDL_ENABLE
                                                      : SDL_DISABLE);
      SDL_Color background = current_screen_->GetBackgroundColor();
      SDL_SetRenderDrawColor(renderer_, background.r, background.g,
                             background.b, 0xff);
      presentation_countdown_ = 2;
      current_screen_->IsActive();
    }

    SDL_RenderClear(renderer_);
    if (current_screen_) {
      current_screen_->Render();
    }

    SDL_RenderPresent(renderer_);
    if (presentation_countdown_ > 0) {
      presentation_countdown_ -= 1;
      if (presentation_countdown_ == 1) {
        if (previous_screen_ != nullptr) {
          previous_screen_->IsInvisible();
        }
      } else if (presentation_countdown_ == 0) {
        current_screen_->IsVisible();
      }
    }
  }

  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);

  SDL_Quit();
}

}  // namespace stimulus
