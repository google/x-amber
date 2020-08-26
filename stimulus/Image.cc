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

#include "Image.h"
#include <SDL_image.h>
#include <csetjmp>
#include "Platform.h"
#include "Screen.h"

namespace stimulus {

SDL_Texture *LoadImage(const std::string &path) {
  SDL_Surface *surface = IMG_Load(path.c_str());

  SDL_Texture *texture = SDL_CreateTextureFromSurface(Screen::GetRenderer(),
      surface);
  if (texture == nullptr) {
    SDL_FreeSurface(surface);
    Screen::FatalError(
        std::string("Image::Image: SDL_CreateTextureFromSurface: ")
        + SDL_GetError());
    return nullptr;
  }

  SDL_FreeSurface(surface);
  return texture;
}

}  // namespace stimulus
