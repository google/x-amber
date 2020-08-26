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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_FLANKERS_ENGINE_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_FLANKERS_ENGINE_H_

#include <memory>
#include <vector>
#include "SDL.h"

#include "Shuffler.h"

namespace stimulus {

struct FlankersStimulus {
  int mark;
  char expected_response;
  std::shared_ptr<SDL_Texture> texture;
  std::string resource_name;
  std::string event;
};

const char kCharLeft = 's';
const char kCharRight = 'l';
const char kCharNone = '\0';

class FlankersEngine {
 public:
  FlankersEngine(const FlankersStimulus stimuli_list[], int num_stimuli,
                 int trials_per_stimuli, int max_run);
  virtual ~FlankersEngine();

  void Reset();

  const FlankersStimulus *GetNextTrial();
  void RecordKeyPress(char);

  // Returns the percentage of errors (0 - 100) of the last set of trials.
  int GetErrorPercent(unsigned num_trials);
  int GetTrialCount() { return trial_count_; }

 private:
  unsigned trial_count_ = 0;
  std::vector<const FlankersStimulus *> trials_;
  std::vector<char> keys_;
  Shuffler<const FlankersStimulus *> shuffler_;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_FLANKERS_ENGINE_H_
