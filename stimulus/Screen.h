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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SCREEN_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SCREEN_H_

#include <SDL.h>
#include <string>
#include <vector>

namespace stimulus {

namespace {
const float kDefaultFontScale = 1.0;
}

class Screen {
 public:
  virtual ~Screen() = default;

  // A successor is a screen this one can transition to by calling
  // SwitchToScreen.
  void AddSuccessor(Screen *screen) {
    successors_.push_back(screen);
  }

  static bool InitDisplay(float screen_width_cm, float screen_height_cm);
  static void MainLoop(Screen *initial_screen);
  static SDL_Renderer *GetRenderer();
  static void FatalError(const std::string &error);

 protected:
  void SwitchToScreen(int successor_num, int delay_ms = 0);

  // Draw the screen. This is called at the frame rate, so should avoid
  // anything compute intensive.
  virtual void Render() {}

  // Called before the first render when a screen is first switched to.
  virtual void IsActive() {}

  // This is intended to be call as soon as the rendered frame is visible.
  // It will be called exactly once each time a screen is switched to.
  virtual void IsVisible() {}

  // This is intended to be called as soon as the next rendered frame is
  // visible. It will be called exactly once each time a screen is switched
  // to, right before the next screen's IsVisible()
  virtual void IsInvisible() {}

  // Called when a screen is about to switch away. Called before the
  // next screen's IsActive(), but before the current screen's IsInvisible()
  virtual void IsInactive() {}

  virtual void KeyPressed(SDL_Scancode) {}
  virtual void MouseClicked(int button, int x, int y) {}

  // Blit the texture in the center of the screen.
  static void Blit(SDL_Texture *texture);

  // Blit the texture, unscaled, at the given pixel coordinates
  static void Blit(SDL_Texture *texture, int left, int top);

  // Blit the texture to the given destination rect.
  static void Blit(SDL_Texture *texture, const SDL_Rect &dest_rect);

  static void DrawRect(const SDL_Rect &rect, const SDL_Color &line_color,
                       const SDL_Color &bg_color);

  static void FillRect(const SDL_Rect &rect, const SDL_Color &fill_color,
                       const SDL_Color &bg_color);

  static void DrawString(int left, int top, const std::string &str,
                         float font_scale = kDefaultFontScale);
  static int GetStringWidth(const std::string &str,
                            float font_scale = kDefaultFontScale);
  static int GetFontHeight(float font_scale = kDefaultFontScale);

  int GetNumSuccessors() const {
    return successors_.size();
  }

  // This is expected to be called from the constructor.
  void SetCursorVisible(bool visible) {
    cursor_visible_ = visible;
  }

  virtual SDL_Color GetBackgroundColor() {
    return SDL_Color{ 0x60, 0x60, 0x60, 0xff };
  }

  virtual SDL_Color GetLineColor() { return SDL_Color{0xff, 0xff, 0xff, 0xff}; }

  static int HorzSizeToPixels(float width_cm) {
    return width_cm * horz_pixels_per_cm_;
  }

  static int VertSizeToPixels(float height_cm) {
    return height_cm * vert_pixels_per_cm_;
  }

  // Compute a rect that will center this texture on the screen and
  // make the width be a fixed physical width on the screen.
  static SDL_Rect ComputeRectForPhysicalWidth(SDL_Texture *texture,
      float width_cm);

  // Compute a rect that will center this texture on the screen and
  // make the height be a fixed physical height on the screen.
  static SDL_Rect ComputeRectForPhysicalHeight(SDL_Texture *texture,
                                               float height_cm);

  static int GetDisplayWidthPx() {
    return display_width_px_;
  }

  static int GetDisplayHeightPx() {
    return display_height_px_;
  }

  static float GetPixelRatio() {
    return pixel_ratio_;
  }

  static SDL_Rect CenterRect(int width, int height) {
    SDL_Rect result;
    result.w = width;
    result.h = height;
    result.x = (display_width_px_ - width) / 2;
    result.y = (display_height_px_ - height) / 2;

    return result;
  }

  static SDL_Rect CenterRectInRect(int width, int height,
                                   const SDL_Rect &rect) {
    SDL_Rect result;
    result.w = width;
    result.h = height;
    result.x = rect.x + (rect.w - width) / 2;
    result.y = rect.y + (rect.h - height) / 2;

    return result;
  }

  static SDL_Rect CenterRectInRect(const SDL_Rect &src_rect,
                                   const SDL_Rect &container_rect) {
    SDL_Rect result;
    result.w = src_rect.w;
    result.h = src_rect.h;
    result.x = container_rect.x + (container_rect.w - src_rect.w) / 2;
    result.y = container_rect.y + (container_rect.h - src_rect.h) / 2;

    return result;
  }

  static SDL_Point CenterString(const std::string &str,
                                float font_scale = kDefaultFontScale) {
    SDL_Point result;
    result.x = (GetDisplayWidthPx() - GetStringWidth(str, font_scale)) / 2;
    result.y = (GetDisplayHeightPx() - GetFontHeight(font_scale)) / 2;

    return result;
  }

  static SDL_Point CenterStringInRect(const std::string &str, SDL_Rect rect,
                                      float font_scale = kDefaultFontScale) {
    SDL_Point result;
    result.x = rect.x + (rect.w - GetStringWidth(str, font_scale)) / 2;
    result.y = rect.y + (rect.h - GetFontHeight(font_scale)) / 2;

    return result;
  }

  static bool IsHit(int x, int y, SDL_Rect &rect) {
    int display_x = static_cast<int>(static_cast<float>(x) * GetPixelRatio());
    int display_y = static_cast<int>(static_cast<float>(y) * GetPixelRatio());
    return (display_x >= rect.x) && (display_x <= rect.x + rect.w) &&
           (display_y >= rect.y) && (display_y <= rect.y + rect.h);
  }

 private:
  std::vector<Screen*> successors_;
  bool cursor_visible_ = false;

  static float horz_pixels_per_cm_;
  static float vert_pixels_per_cm_;
  static Screen *previous_screen_;
  static Screen *current_screen_;
  static Screen *next_screen_;
  static SDL_Renderer *renderer_;
  static SDL_Window *window_;
  static int next_screen_presentation_time_;
  static int presentation_countdown_;
  static int display_width_px_;
  static int display_height_px_;
  static float pixel_ratio_;
  static SDL_Texture *font_atlas_;
  static float font_scale_;
  static int glyph_height_;
  static bool enable_sdl_error_dialog_;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SCREEN_H_
