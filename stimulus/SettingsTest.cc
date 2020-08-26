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

#include "Settings.h"

#include <boost/test/unit_test.hpp>

namespace {

const std::string kGoodSettings = "test-settings-good.txt";
const std::string kMissingValueSettings = "test-settings-missing-value.txt";

BOOST_AUTO_TEST_CASE(HasKey) {
  stimulus::Settings settings(kGoodSettings);
  BOOST_CHECK(settings.HasKey("foo"));
  BOOST_CHECK(settings.HasKey("bar"));
  BOOST_CHECK(settings.HasKey("zzz"));
  BOOST_CHECK(!settings.HasKey("abc"));
}

BOOST_AUTO_TEST_CASE(GetValue) {
  stimulus::Settings settings(kGoodSettings);
  BOOST_CHECK_EQUAL(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_\\-/:%",
      settings.GetValue("foo"));
  BOOST_CHECK_EQUAL("baz", settings.GetValue("bar"));
  BOOST_CHECK_EQUAL("xxx", settings.GetValue("zzz"));
}

BOOST_AUTO_TEST_CASE(GetFloat) {
  stimulus::Settings settings(kGoodSettings);
  BOOST_CHECK_EQUAL(123.5, settings.GetFloatValue("aaa"));
}

BOOST_AUTO_TEST_CASE(GetInt) {
  stimulus::Settings settings(kGoodSettings);
  BOOST_CHECK_EQUAL(6789, settings.GetIntValue("bbb"));
}

BOOST_AUTO_TEST_CASE(ValidFile) {
  stimulus::Settings settings(kGoodSettings);
  BOOST_CHECK_EQUAL("", settings.GetErrors());
}

BOOST_AUTO_TEST_CASE(MissingValue) {
  stimulus::Settings settings(kMissingValueSettings);
  BOOST_CHECK_EQUAL("line 3: missing value\nline 5: missing value\n",
      settings.GetErrors());
}

BOOST_AUTO_TEST_CASE(NoFile) {
  stimulus::Settings settings("/this-file-should-not-exist.txt");
  BOOST_CHECK_EQUAL("Could not open settings file", settings.GetErrors());
}

}  // namespace

