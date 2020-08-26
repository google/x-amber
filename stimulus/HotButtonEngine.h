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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_HOTBUTTON_HOTBUTTONENGINE_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_HOTBUTTON_HOTBUTTONENGINE_H_

namespace stimulus {

const int kHotButtonWinPercent12 = 12;
const int kHotButtonWinPercent50 = 50;
const int kHotButtonWinPercent88 = 88;

const int kHotButtonPoints0 = 0;
const int kHotButtonPoints100 = 100;
const int kHotButtonPoints300 = 300;
const int kHotButtonPoints500 = 500;

struct HotButtonTrial {
  int easy_points;
  int hard_points;
  int win_probability_pct;
};

class HotButtonEngine {
 public:
  virtual ~HotButtonEngine() {}

  void SetLeftHandedness(bool left_handed) { left_handed_ = left_handed; }
  void SetStartTime(int start_time) { start_time_ = start_time; }
  void SetEasyTrial(bool easy) { next_trial_easy_ = easy; }

  bool GetLeftHandedness() { return left_handed_; }
  bool GetEasyTrial() { return next_trial_easy_; }
  bool GetLastTrialSuccess() { return last_trial_success_; }
  int GetLastTrialPoints() { return last_won_points_; }
  int GetPotentialTotalPoints() { return potential_total_points_; }
  int GetTotalPoints() { return total_points_; }
  int GetStartTime() { return start_time_; }
  int GetTrialNumKeypresses();
  int GetTrialTimeout();

  virtual HotButtonTrial GetNextTrial();

  void SucceedTrial();
  void FailTrial();

  void Reset() {
    total_points_ = 0;
    potential_total_points_ = 0;
  };

 private:
  // task properties
  bool left_handed_;
  int start_time_;
  int total_points_ = 0;
  int potential_total_points_;
  // trial properties
  bool last_trial_success_;
  int last_won_points_;
  bool next_trial_easy_;
  HotButtonTrial next_trial_;
};

class HotButtonDemoEngine : public HotButtonEngine {
 public:
  virtual ~HotButtonDemoEngine() {}
  virtual HotButtonTrial GetNextTrial() override;

 private:
  int count_ = 0;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_HOTBUTTON_HOTBUTTONENGINE_H_
