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

#include "FlankersEngine.h"
#include <cassert>
#include <iostream>

namespace stimulus {

FlankersEngine::FlankersEngine(const FlankersStimulus stimuli_list[],
                               int num_stimuli, int trials_per_stimuli,
                               int max_run) {
  // Use Shuffler class to ensure we generate exactly the number of required
  // stimuli, and that no rare stimuli is presented more than three times in a
  // row.
  for (int i = 0; i < num_stimuli; i++) {
    std::vector<const FlankersStimulus *> stimuli(trials_per_stimuli);
    std::fill(stimuli.begin(), stimuli.end(), &stimuli_list[i]);
    shuffler_.AddCategoryElements(stimuli, max_run);
  }
  shuffler_.ShuffleElements();
}

FlankersEngine::~FlankersEngine() {}

void FlankersEngine::Reset() {
  trial_count_ = 0;
  trials_.clear();
  keys_.clear();
  shuffler_.ShuffleElements();
}

const FlankersStimulus *FlankersEngine::GetNextTrial() {
  assert(trials_.size() == keys_.size());
  assert(!shuffler_.IsDone());

  const FlankersStimulus *next = shuffler_.GetNextItem();
  trials_.push_back(next);
  trial_count_ += 1;

  assert(trials_.size() == trial_count_);
  return next;
}

void FlankersEngine::RecordKeyPress(char c) {
  keys_.push_back(c);
  assert(keys_.size() == trials_.size());
}

int FlankersEngine::GetErrorPercent(unsigned num_trials) {
  assert(trials_.size() >= num_trials);
  assert(keys_.size() >= num_trials);

#if DEBUG
  std::cout << "===============" << std::endl;
  std::cout << "Last " << num_trials << " trials:" << std::endl;
  std::cout << "---------------" << std::endl;
#endif

  int error_count = 0;
  for (unsigned i = trials_.size() - num_trials; i < trials_.size(); i++) {
    if (trials_[i]->expected_response != keys_[i] && keys_[i] != kCharNone) {
      error_count += 1;
    }
#if DEBUG
    std::cout << i << "\t" << trials_[i]->expected_response << "\t" << keys_[i]
              << "\t" << trials_[i]->event << std::endl;
#endif
  }

  int error_pct = error_count * 100 / num_trials;

#if DEBUG
  std::cout << "---------------" << std::endl;
  std::cout << "error_count " << error_count << " error_pct " << error_pct
            << std::endl;
  std::cout << "===============" << std::endl;
#endif

  return error_pct;
}

}  // namespace stimulus
