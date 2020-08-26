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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_EMOTIONALIMAGES_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_EMOTIONALIMAGES_H_

#include "Screen.h"
#include "Settings.h"

namespace stimulus {

Screen *InitEmotionalImages(Screen*, const Settings &settings);

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_EMOTIONALIMAGES_H_
