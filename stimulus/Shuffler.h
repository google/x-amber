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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SHUFFLER_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SHUFFLER_H_

#include <deque>
#include <vector>
#include "Random.h"

#include <iostream>

namespace stimulus {

template <typename T>
class Shuffler {
 public:
  void ShuffleElements() {
    do {
      shuffled_elements_ = TryToShuffleElements();
    } while (shuffled_elements_.empty());
  }

  bool IsDone() const {
    return shuffled_elements_.empty();
  }

  T GetNextItem() {
    T retval = shuffled_elements_.front();
    shuffled_elements_.pop_front();
    return retval;
  }

  void AddCategoryElements(const std::vector<T> &category_elements,
                           int max_run_length) {
    int category = category_counts_.size();
    for (auto element : category_elements) {
      Element dest_element = {category, max_run_length, element};
      source_elements_.push_back(dest_element);
    }

    category_counts_.push_back(category_elements.size());
    if (!category_elements.empty()) {
      num_non_empty_categories_++;
    }
  }

 private:
  struct Element {
    int category;
    int max_run_length;
    T value;
  };

  std::deque<T> TryToShuffleElements() {
    std::vector<Element> source = source_elements_;
    std::vector<int> category_counts = category_counts_;
    int num_non_empty_categories = num_non_empty_categories_;
    int last_category = -1;
    int run_length = 1;
    std::deque<T> result;

    while (!source.empty()) {
      int draw_index;
      do {
        draw_index = GenerateRandomInt(0, source.size());
      } while (num_non_empty_categories > 1 &&
               source[draw_index].max_run_length > 0 &&
               run_length == source[draw_index].max_run_length &&
               source[draw_index].category == last_category);

      int category = source[draw_index].category;
      if (category == last_category) {
        if (source[draw_index].max_run_length > 0 &&
            ++run_length > source[draw_index].max_run_length) {
          // Failed to generate correct sequence because there is a run at the end.
          // Return an empty vector to indicate this.
          return std::deque<T>();
        }
      } else {
        last_category = category;
        run_length = 1;
      }

      // We'd get into an infinite loop if we tried to avoid a run with only
      // one category left. Detect that here.
      if (--category_counts[category] == 0) {
        num_non_empty_categories--;
      }

      result.push_back(source[draw_index].value);
      source.erase(source.begin() + draw_index);
    }

    return result;
  }

  std::vector<Element> source_elements_;
  std::deque<T> shuffled_elements_;
  std::vector<int> category_counts_;
  int num_non_empty_categories_ = 0;
  bool shuffled_ = false;
};

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SHUFFLER_H_
