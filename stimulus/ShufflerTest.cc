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
#include "Shuffler.h"

#include <boost/test/unit_test.hpp>

namespace {

const int kElementsPerCategory = 100;
const int kNumCategories = 4;
const int kMaxRunLength = 3;

// Each category should have no more than a run of 3
BOOST_AUTO_TEST_CASE(Shuffle) {
  stimulus::InitRandom(time(nullptr));

  stimulus::Shuffler<int> shuffler;
  for (int i = 0; i < kNumCategories; i++) {
    std::vector<int> values;
    for (int j = 0; j < kElementsPerCategory; j++) {
      values.push_back(i * kElementsPerCategory + j);
    }

    shuffler.AddCategoryElements(values, kMaxRunLength);
  }

  shuffler.ShuffleElements();

  bool seen_flags[kElementsPerCategory * kNumCategories];
  memset(seen_flags, 0, sizeof(seen_flags));
  int last_category = -1;
  int run_length = 1;
  for (int i = 0; i < kElementsPerCategory * kNumCategories; i++) {
    BOOST_CHECK(!shuffler.IsDone());
    int value = shuffler.GetNextItem();
    int category = value / kElementsPerCategory;
    if (category == last_category) {
      run_length++;
      BOOST_CHECK_LE(run_length, kMaxRunLength);
    } else {
      run_length = 1;
      last_category = category;
    }

    // Ensure no duplicates (and by extension that every item appears once).
    BOOST_CHECK(!seen_flags[value]);
    seen_flags[value] = true;
  }

  BOOST_CHECK(shuffler.IsDone());
}

// Only the first category has a max run restriction
BOOST_AUTO_TEST_CASE(Shuffle2) {
  stimulus::InitRandom(time(nullptr));

  stimulus::Shuffler<int> shuffler;
  for (int i = 0; i < kNumCategories; i++) {
    std::vector<int> values;
    for (int j = 0; j < kElementsPerCategory; j++) {
      values.push_back(i * kElementsPerCategory + j);
    }

    shuffler.AddCategoryElements(values, i == 0 ? kMaxRunLength : 0);
  }

  shuffler.ShuffleElements();

  bool seen_flags[kElementsPerCategory * kNumCategories];
  memset(seen_flags, 0, sizeof(seen_flags));
  int last_category = -1;
  int run_length = 1;
  for (int i = 0; i < kElementsPerCategory * kNumCategories; i++) {
    BOOST_CHECK(!shuffler.IsDone());
    int value = shuffler.GetNextItem();
    int category = value / kElementsPerCategory;
    if (category == last_category) {
      if (category == 0) {
        run_length++;
        BOOST_CHECK_LE(run_length, kMaxRunLength);
      }
    } else {
      run_length = 1;
      last_category = category;
    }

    // Ensure no duplicates (and by extension that every item appears once).
    BOOST_CHECK(!seen_flags[value]);
    seen_flags[value] = true;
  }

  BOOST_CHECK(shuffler.IsDone());
}

// Compute the distribution of elements in each list position. These should all
// be relatively even.
BOOST_AUTO_TEST_CASE(BucketDistribution) {
  const int kNumTrials = 1000000;
  const int kNumElements = 10;
  const std::vector<int> kElements1 { 0,1,2,3,4 };
  const std::vector<int> kElements2 { 5,6,7,8,9 };
  const int kTarget = kNumTrials / kNumElements;

  // Ensure these don't differ by more than 2%
  const int kMaxHist = kTarget * 102 / 100;
  const int kMinHist = kTarget * 98 / 100;
  int position_distribution[kNumElements][kNumElements];

  memset(position_distribution, 0, sizeof(position_distribution));

  for (int rep = 0; rep < kNumTrials; rep++) {
    stimulus::Shuffler<int> shuffler;
    shuffler.AddCategoryElements(kElements1, kMaxRunLength);
    shuffler.AddCategoryElements(kElements2, kMaxRunLength);
    shuffler.ShuffleElements();
    for (int i = 0; i < kNumElements; i++) {
      int item = shuffler.GetNextItem();
      position_distribution[i][item]++;
    }
  }

  for (int position = 0; position < kNumElements; position++) {
    for (int value = 0; value < kNumElements; value++) {
      int count = position_distribution[position][value];
      BOOST_CHECK_GT(count, kMinHist);
      BOOST_CHECK_LT(count, kMaxHist);
    }
  }
}

}  // namespace
