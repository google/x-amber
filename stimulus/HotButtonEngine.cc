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

#include "HotButtonEngine.h"

#include "Random.h"

namespace stimulus {
namespace {

const int kNumKeypressesEasy = 21;
const int kNumKeypressesHard = 71;
const int kTrialTimeoutEasy = 5000;
const int kTrialTimeoutHard = 15000;

}  // namespace

HotButtonTrial HotButtonEngine::GetNextTrial() {
  next_trial_.easy_points = kHotButtonPoints100;
  double r = GenerateRandomDouble();
  if (r < 0.5) {
    next_trial_.hard_points = kHotButtonPoints300;
  } else {
    next_trial_.hard_points = kHotButtonPoints500;
  }

  r = GenerateRandomDouble();
  if (r < 0.33) {
    next_trial_.win_probability_pct = kHotButtonWinPercent12;
  } else if (r < 0.66) {
    next_trial_.win_probability_pct = kHotButtonWinPercent50;
  } else {
    next_trial_.win_probability_pct = kHotButtonWinPercent88;
  }
  potential_total_points_ += next_trial_.hard_points;
  return next_trial_;
}

HotButtonTrial DemoTrials[3] = {
    {
        /*.easy_points = */ kHotButtonPoints100,
        /*.hard_points = */ kHotButtonPoints500,
        /*.win_probability_pct = */ kHotButtonWinPercent50,
    },
    {
        /*.easy_points = */ kHotButtonPoints100,
        /*.hard_points = */ kHotButtonPoints300,
        /*.win_probability_pct = */ kHotButtonWinPercent12,
    },
    {
        /*.easy_points = */ kHotButtonPoints100,
        /*.hard_points = */ kHotButtonPoints500,
        /*.win_probability_pct = */ kHotButtonWinPercent88,
    },
};

HotButtonTrial HotButtonDemoEngine::GetNextTrial() {
  return DemoTrials[(count_++ % 3)];
}

int HotButtonEngine::GetTrialNumKeypresses() {
  return next_trial_easy_ ? kNumKeypressesEasy : kNumKeypressesHard;
}

int HotButtonEngine::GetTrialTimeout() {
  return next_trial_easy_ ? kTrialTimeoutEasy : kTrialTimeoutHard;
}

void HotButtonEngine::SucceedTrial() {
  last_trial_success_ = true;
  double r = GenerateRandomDouble();
  if (r * 100 < next_trial_.win_probability_pct) {
    last_won_points_ =
        next_trial_easy_ ? next_trial_.easy_points : next_trial_.hard_points;
    total_points_ += last_won_points_;
  } else {
    last_won_points_ = 0;
  }
}

void HotButtonEngine::FailTrial() {
  last_trial_success_ = false;
  last_won_points_ = 0;
}

}  // namespace stimulus
