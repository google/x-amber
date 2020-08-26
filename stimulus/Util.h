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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_UTIL_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_UTIL_H_

#include <SDL.h>

#include <cstdint>
#include <string>
#include <vector>

namespace stimulus {

// Compare two integers, handling the wrap case.
inline bool WrappedGreaterEqual(uint32_t a, uint32_t b) {
  return (a < b && b - a > 0x7fffffff) || (a > b && a - b < 0x7fffffff) ||
         (a == b);
}

inline SDL_Rect InsetRect(const SDL_Rect &src, int x, int y) {
  SDL_Rect result{src.x + x / 2, src.y + y / 2, src.w - x, src.h - y};
  return result;
}

// std::shared_ptr<SDL_Texture> deleter
struct SDL_TextureDeleter {
  void operator()(SDL_Texture *p) const { SDL_DestroyTexture(p); }
};

struct RenderString {
  std::string str;
  int x;
  int y;
};

int LineBreak(std::vector<RenderString> &out_vec, std::string str,
              unsigned max_chars, int y, int font_height);


}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_UTIL_H_
