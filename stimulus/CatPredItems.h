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

#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_CATPREDITEMS_H_
#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_CATPREDITEMS_H_

namespace stimulus {

struct CatPredItem {
  int cue_mark;
  const char *cue;
  const char *high_typicality;
  const char *low_typicality;
  const char *incongruent;
};

// clang-format off
/* ﻿itemID, cue, HighTypicality, LowTypicality, Incongruent */
const CatPredItem CatPredItems[] = {
  { 2, "A carpenter's tool", "hammer", "square", "cellar" },
  { 3, "A common color", "yellow", "silver", "tunnel" },
  { 4, "A misdemeanor crime", "speeding", "trespassing", "eating" },
  { 5, "A non-fatal disease", "measles", "asthma", "freight" },
  { 6, "A four-footed animal", "cat", "turtle", "panic" },
  { 7, "A kitchen utensil", "spatula", "sponge", "harmony" },
  { 9, "A natural earth formation", "mountain", "island", "plastic" },
  { 10, "A non-alcoholic beverage", "tea", "punch", "cart" },
  { 11, "A part of the body", "leg", "throat", "keys" },
  { 13, "A kind of snake", "cobra", "anaconda", "academy" },
  { 14, "A type of spice", "pepper", "ginger", "rounds" },
  { 15, "A type of condiment", "ketchup", "horseradish", "fraction" },
  { 17, "A kind of tree", "oak", "ash", "tin" },
  { 18, "A type of music", "jazz", "march", "crew" },
  { 19, "A type of reading material", "magazine", "poem", "ration" },
  { 20, "An air vehicle", "airplane", "balloon", "storage" },
  { 21, "A land vehicle", "bus", "van", "pod" },
  { 22, "A non-green vegetable", "carrot", "turnip", "mother" },
  { 23, "An elective office", "senator", "commissioner", "hamburger" },
  { 24, "A piece of furniture", "couch", "cabinet", "rivals" },
  { 25, "A type of bird", "robin", "duck", "virus" },
  { 26, "A kind of cheese", "cheddar", "brie", "flame" },
  { 27, "A kind of chemical", "chlorine", "iodine", "quarter" },
  { 28, "A type of cosmetic", "lipstick", "cologne", "trailer" },
  { 29, "A type of dance", "waltz", "tap", "bait" },
  { 30, "A deadly disease", "cancer", "rabies", "stream" },
  { 32, "An eating utensil", "fork", "bowl", "oval" },
  { 33, "A positive emotion", "happiness", "relief", "birthday" },
  { 34, "A negative emotion", "anger", "guilt", "clamp" },
  { 35, "A kind of fish", "trout", "marlin", "yield" },
  { 36, "A type of flower", "rose", "poppy", "opera" },
  { 37, "A type of government", "democracy", "aristocracy", "skateboard" },
  { 38, "A type of fruit", "apple", "cherry", "penny" },
  { 40, "A board game", "chess", "backgammon", "dilemma" },
  { 41, "A kind of insect", "ant", "hornet", "gate" },
  { 42, "An American island", "Hawaii", "Manhattan", "Calendar" },
  { 43, "A foreign language", "Russian", "Dutch", "Attire" },
  { 44, "A type of meat", "pork", "turkey", "bench" },
  { 45, "A kind of metal", "copper", "uranium", "equator" },
  { 46, "A kind of mineral", "quartz", "graphite", "chamber" },
  { 47, "A percussion instrument", "drum", "tambourine", "flight" },
  { 48, "A human organ", "lung", "skin", "joke" },
  { 49, "A type of profession", "lawyer", "author", "jargon" },
  { 50, "A kind of religion", "Christianity", "Anglican", "Spotlight" },
  { 51, "A kind of reptile", "alligator", "chameleon", "explosion" },
  { 52, "A type of sport", "basketball", "skiing", "musician" },
  { 53, "A type of ship", "cruiser", "ferry", "waiter" },
  { 54, "A type of bread", "rye", "sourdough", "podium" },
  { 56, "A human dwelling", "apartment", "cabin", "feather" },
  { 57, "A male relative", "uncle", "nephew", "lumber" },
  { 58, "A type of tax", "income", "estate", "sleeve" },
  { 59, "A type of weapon", "gun", "missile", "juror" },
  { 60, "A weather phenomenon", "hurricane", "sandstorm", "parachute" },
  { 61, "A bird of prey", "eagle", "owl", "veil" },
  { 62, "A breed of dog", "poodle", "pointer", "marble" },
  { 63, "A building material", "wood", "stone", "topic" },
  { 65, "A dairy product", "cheese", "cream", "artist" },
  { 66, "A farm animal", "cow", "goat", "bolt" },
  { 68, "A gardening tool", "rake", "gloves", "ivory" },
  { 70, "A part of speech", "verb", "preposition", "juggler" },
  { 71, "A green vegetable", "lettuce", "cabbage", "patient" },
  { 72, "A type of herb", "oregano", "ginseng", "dispute" },
  { 73, "A kind of liquor", "whiskey", "brandy", "ceiling" },
  { 74, "A major appliance", "refrigerator", "range", "humanity" },
  { 76, "A female member of royalty", "princess", "duchess", "nursery" },
  { 77, "A brass instrument", "trumpet", "trombone", "antenna" },
  { 78, "A string instrument", "guitar", "piano", "fence" },
  { 79, "A wind instrument", "flute", "oboe", "pint" },
  { 80, "A mythical being", "unicorn", "troll", "arcade" },
  { 81, "A piece of farm equipment", "tractor", "shovel", "comedy" },
  { 82, "A piece of firefighting equipment", "hose", "axe", "coal" },
  { 83, "A piece of horse-riding equipment", "saddle", "bit", "lever" },
  { 84, "A piece of jewelry", "ring", "watch", "spear" },
  { 85, "A kind of rodent", "mouse", "gopher", "ritual" },
  { 86, "A part of a house", "bedroom", "basement", "dictator" },
  { 87, "A train car", "caboose", "sleeper", "monthly" },
  { 88, "A two-wheeled vehicle", "bicycle", "chariot", "suspect" },
  { 90, "A type of cloth", "silk", "satin", "kite" },
  { 91, "A type of fastener", "button", "nail", "movie" },
  { 92, "A type of hat", "cap", "bonnet", "tire" },
  { 93, "A type of nut", "walnut", "chestnut", "needle" },
  { 94, "A type of reference book", "encyclopedia", "almanac", "projectile" },
  { 96, "A wild animal", "lion", "squirrel", "region" },
  { 97, "A writing implement", "pen", "chalk", "toll" },
  { 98, "A cleaning instrument", "broom", "towel", "porch" },
  { 99, "A type of dessert", "cake", "fudge", "blow" },
  { 101, "A kind of jam", "strawberry", "pineapple", "insurance" },
  { 102, "A kind of juice", "grape", "pear", "nest" },
  { 103, "A kind of seafood", "lobster", "squid", "prison" },
  { 104, "A kitchen appliance", "stove", "freezer", "ticket" },
  { 105, "An item made of leather", "belt", "hat", "race" },
  { 106, "A kind of liquid", "water", "paint", "slate" },
  { 107, "A piece of camping equipment", "tent", "compass", "smoke" },
  { 109, "A scientific instrument", "microscope", "laser", "seaweed" },
  { 110, "A surgical tool", "scalpel", "ether", "planet" },
  { 111, "A type of building", "store", "museum", "muffler" },
  { 112, "A water sport", "swimming", "rowing", "pottery" },
  { 113, "A type of women's clothing", "skirt", "gown", "poker" },
  { 114, "A type of men's clothing", "tie", "vest", "soup" },
  { 115, "A flavor of ice-cream", "vanilla", "pistachio", "lighter" },
  { 116, "An instrument of war", "tank", "submarine", "cooling" },
  { 117, "An office supply", "pencil", "envelope", "oatmeal" },
  { 118, "A piece of sports equipment", "bat", "net", "rot" },
  { 120, "An optical instrument", "telescope", "mirror", "counter" },
  { 122, "An annual season", "fall", "hunting", "force" },
  { 123, "A part of a tree", "branch", "twig", "plaza" },
  { 124, "A type of green thing", "grass", "frog", "giant" },
  { 125, "A part of a bicycle", "wheel", "lock", "store" },
  { 126, "A type of wood", "oak", "elm", "key" },
  { 127, "A piece of hiking equipment", "boots", "jacket", "track" },
  { 128, "A thing that flies", "bird", "bat", "rice" },
  { 129, "A brand of car", "Ford", "Saturn", "Puppet" },
  { 134, "A type of science", "biology", "anthropology", "cream" },
  { 135, "A thing made of wood", "table", "paper", "icicle" },
  { 137, "A thing that makes noise", "car", "alarm", "order" },
  { 139, "A part of a building", "window", "lobby", "glove" },
  { 141, "A bathroom fixture", "toilet", "lamp", "fang" },
  { 142, "A place of burial", "cemetery", "crypt", "thread" },
  { 143, "A branch of government", "executive", "military", "ladybug" },
  { 144, "A time of day", "noon", "twilight", "wilderness" },
};
// clang-format on

}  // namespace stimulus

#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_CATPREDITEMS_H_
