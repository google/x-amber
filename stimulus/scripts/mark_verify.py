# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Verifies mark output file

import json
import logging
import os
import sys

from functools import reduce

logging.basicConfig(level=logging.INFO)

def TestEqual(expected, actual, name):
  res = False
  logging.info("%s %s expected %s", name, str(actual), str(expected))
  if actual == expected:
    res = True
    logging.info("  PASS")
  else:
    res = False
    logging.info("  FAIL")
  return res

def OddballVerify(marks, sorted):
  NSTANDARD_EXPECTED = 320
  NODDBALL_EXPECTED = 80
  NBREAK_EXPECTED = 9
  KEYS_EXPECTED = ["1", "2", "777", "888", "999"]
  FIRST_MARK_EXPECTED = "999"
  LAST_MARK_EXPECTED = "999"

  logging.info("---------------------")
  logging.info("Task: Oddball")
  logging.info("---------------------")

  nstandard = len(sorted["1"])
  TestEqual(NSTANDARD_EXPECTED, nstandard, "nstandard")

  noddball = len(sorted["2"])
  TestEqual(NODDBALL_EXPECTED, noddball, "noddball")

  nbreak = len(sorted["888"])
  TestEqual(NBREAK_EXPECTED, nbreak, "nbreak")

  keys = list(sorted.keys())
  keys.sort()
  TestEqual(KEYS_EXPECTED, keys, "marks")

  first_mark = marks[0]["mark"]
  TestEqual(FIRST_MARK_EXPECTED, first_mark, "first_mark")

  last_mark = marks[len(marks)-1]["mark"]
  TestEqual(LAST_MARK_EXPECTED, last_mark, "last_mark")

  prev_ts = 0
  mark_latencies = []
  for m in marks:
    if m["mark"] == "888":
      prev_ts = 0

    if m["mark"] != "1" and m["mark"] != "2":
      continue

    ts = int(m["ts"])
    if prev_ts != 0:
      mark_latencies.append(ts - prev_ts)
    prev_ts = ts

  total_latencies = reduce(lambda x, y: x + y, mark_latencies)
  logging.info("average time between marks %d ms", total_latencies / len(mark_latencies))

def SsvepVerify(marks, sorted):
  NCONDITIONS_EXPECTED = 60
  CONDITION_MARKS = ["123", "132", "213", "231", "312", "321"]
  CONDITION_MARKS.sort()
  IMAGE_MARK_LOW = 1000
  IMAGE_MARK_HIGH = 3999

  logging.info("---------------------")
  logging.info("Task: SSVEP")
  logging.info("---------------------")

  conditions = []

  prev_image_ts = 0
  nimages = 0
  image_time_total = 0

  for m in marks:
    if m["mark"] in CONDITION_MARKS:
      conditions.append(m["mark"])
      prev_image_ts = 0

    if int(m["mark"]) >= IMAGE_MARK_LOW and int(m["mark"]) <= IMAGE_MARK_HIGH:
      ts = int(m["ts"])
      if prev_image_ts == 0:
        prev_image_ts = ts
      else:
        # don't count the first one on purpose - we don't have a time locked
        # mark for the end of the last image so need to discard that one
        nimages += 1
        image_time_total += ts - prev_image_ts
        prev_image_ts = ts

  # TODO: what's the tolerances for this?
  logging.info("average time between images %d ms", image_time_total / nimages)

  TestEqual(NCONDITIONS_EXPECTED, len(conditions), "num_conditions")

  unique = list(set(conditions))
  unique.sort()
  TestEqual(CONDITION_MARKS, unique, "conditions")

def FlankersVerify(marks, sorted):
  FLANKER_MARKS = ["44", "46", "66", "64"]
  OFFSET_MARK = "77"

  logging.info("---------------------")
  logging.info("Task: Flankers")
  logging.info("---------------------")

  prev_stim_ts = 0
  stim_offset_total = 0
  stim_offset_min = 0
  stim_offset_max = 0
  nstims = 0

  for m in marks:
    ts = int(m["ts"])
    if m["mark"] in FLANKER_MARKS:
      prev_stim_ts = ts
      nstims += 1

    if m["mark"] == OFFSET_MARK:
      offset = ts - prev_stim_ts
      stim_offset_total += offset
      if offset > stim_offset_max:
        stim_offset_max = offset
      if stim_offset_min == 0 or offset < stim_offset_min:
        stim_offset_min = offset

  logging.info("average stimulus offset time %d ms min %d ms max %d ms",
    stim_offset_total / nstims, stim_offset_min, stim_offset_max)

if __name__ == "__main__":

  if len(sys.argv) != 2:
    logging.error("Usage:")
    logging.error("  python %s <csvfile>", sys.argv[0])
    sys.exit()

  csvfile = open(sys.argv[1], "r")

  # read and parse header
  header_json = ""
  line = csvfile.readline()
  while line != "----\n":
    header_json += line
    line = csvfile.readline()

  header = json.loads(header_json)
  logging.info(header)

  # skip column header
  csvfile.readline()

  # read and parse csv
  all_marks = []
  sorted_marks = {}
  line = csvfile.readline()
  while line != "":
    s = line.split(",")
    m = {"mark": s[0], "ts": s[1]}
    all_marks.append(m)
    if m["mark"] in sorted_marks:
      sorted_marks[m["mark"]].append(m)
    else:
      sorted_marks[m["mark"]] = [m]

    line = csvfile.readline()

  # task specific tests
  dispatch = {
      "Oddball": OddballVerify,
      "SSVEP": SsvepVerify,
      "Flankers": FlankersVerify,
  }
  dispatch[header["task"]](all_marks, sorted_marks)
