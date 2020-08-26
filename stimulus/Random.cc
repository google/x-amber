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

#include <climits>
#include <cstdint>
#include <cmath>
#include "Random.h"

namespace stimulus {
namespace {
uint32_t random_state[4] = {1,1,1,1};

// https://en.wikipedia.org/wiki/Xorshift
uint32_t Xorshift128() {
  uint32_t t = random_state[3];
  t ^= t << 11;
  t ^= t >> 8;
  for (int i = 3; i > 0; i--) {
    random_state[i] = random_state[i - 1];
  }

  uint32_t s = random_state[0];
  t ^= s;
  t ^= s >> 19;
  random_state[0] = t;

  return t;
}

}  // namespace

void InitRandom(uint32_t seed) {
  random_state[0] = random_state[1] = random_state[2] = random_state[3] = seed;

  for (int i = 0; i < 50; i++) {
    Xorshift128();
  }
}

// Generate a random integer in the range [min, max)
uint32_t GenerateRandomInt(uint32_t min, uint32_t max) {
  // Use floor to ensure this is unbiased
  return floor(GenerateRandomDouble() * (max - min)) + min;
}

double GenerateRandomDouble() {
  // Note: XorShift128 will never return 0. Subtracting 1 is an easy way
  // to ensure this function will never return 1.0.
  return static_cast<double>(Xorshift128() - 1) / UINT_MAX;
}

}  // namespace stimulus
