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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SRET_WORD_LIST_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SRET_WORD_LIST_H_

#include <vector>

const std::vector<std::string> SretNegativeWords{
    "afraid",    "alone",     "angry",      "anguished",  "bored",
    "brutal",    "burdened",  "cruel",      "crushed",    "defeated",
    "depressed", "disgusted", "disloyal",   "displeased", "distressed",
    "dreadful",  "fearful",   "frustrated", "guilty",     "helpless",
    "hostile",   "insane",    "insecure",   "lonely",     "lost",
    "morbid",    "obnoxious", "rejected",   "rude",       "scared",
    "shamed",    "sinful",    "stupid",     "terrible",   "terrified",
    "troubled",  "unhappy",   "upset",      "useless",    "violent",
};

const std::vector<std::string> SretPositiveWords{
    "admired",    "adorable",   "alive",    "beautiful", "bold",
    "bright",     "capable",    "carefree", "confident", "cute",
    "devoted",    "dignified",  "elated",   "engaged",   "famous",
    "festive",    "friendly",   "gentle",   "grateful",  "happy",
    "honest",     "hopeful",    "inspired", "jolly",     "joyful",
    "lively",     "loyal",      "lucky",    "masterful", "outstanding",
    "proud",      "satisfied",  "silly",    "surprised", "terrific",
    "thoughtful", "untroubled", "useful",   "vigorous",  "wise",
};

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_SRET_WORD_LIST_H_
