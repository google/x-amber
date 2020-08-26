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

#include <vector>
#include "Random.h"

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(GenerateRandomInt) {
  stimulus::InitRandom(time(nullptr));

  const int kHistBuckets = 10;
  const int kNumTrials = 100000;
  const int kTarget = kNumTrials / kHistBuckets;

  // Ensure these don't differ by more than 2%
  const int kMaxHist = kTarget * 102 / 100;
  const int kMinHist = kTarget * 98 / 100;
  int histogram[kHistBuckets];

  memset(histogram, 0, sizeof(histogram));
  for (int i = 0; i < kNumTrials; i++) {
    uint32_t value = stimulus::GenerateRandomInt(0, kHistBuckets);
    BOOST_CHECK(value < kHistBuckets);
    histogram[value]++;
  }

  // Verify these are generally in valid statistical bounds.
  for (int i = 0; i < kHistBuckets; i++) {
    BOOST_CHECK(histogram[i] > kMinHist);
    BOOST_CHECK(histogram[i] < kMaxHist);
  }
}

}  // namespace
