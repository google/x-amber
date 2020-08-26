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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_TEXTUREMANAGER_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_TEXTUREMANAGER_H_

#include <string>
#include <unordered_map>

#include "Image.h"

namespace stimulus {

class TextureManager {
 public:
  TextureManager() {}
  ~TextureManager() {
    for (auto &n : map_) {
      SDL_DestroyTexture(n.second);
    }
    map_.clear();
  }

  SDL_Texture *LoadImage(const std::string &filename) {
    if (map_.count(filename) > 0) {
      return map_.at(filename);
    }
    SDL_Texture *texture = stimulus::LoadImage(filename);
    map_.insert({filename, texture});
    return texture;
  }

 private:
  std::unordered_map<std::string, SDL_Texture *> map_;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_TEXTUREMANAGER_UTIL_H_
