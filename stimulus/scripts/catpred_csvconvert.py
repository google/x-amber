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

# Converts catPred_FINAL_1_6_2018.csv to header file

import csv
import logging
import os
import sys

logging.basicConfig(level=logging.INFO)

if __name__ == "__main__":

  if len(sys.argv) != 3:
    logging.error("Usage:")
    logging.error("  python %s <csvfile> <outfile>", sys.argv[0])
    sys.exit()

  csvfile = open(sys.argv[1], "r")
  outfile = open(sys.argv[2], "w")

  string = "\n".join((
    "#ifndef EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_CATPREDITEMS_H_",
    "#define EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_CATPREDITEMS_H_",
    "",
    "namespace stimulus {",
    "",
    "struct CatPredItem {",
    "  int cue_mark;",
    "  const char *cue;",
    "  const char *high_typicality;",
    "  const char *low_typicality;",
    "  const char *incongruent;",
    "};",
    "",
    "// clang-format off\n"
  ))
  outfile.write(string)

  count = 0
  csvreader = csv.reader(csvfile)
  for line in csvreader:
    if count == 0:
      outfile.write("/* " + ", ".join(line) + " */\n")
      outfile.write("const CatPredItem CatPredItems[] = {\n")
    else:
      quoted = tuple(map(lambda s: "\"" + s + "\"", line[1:]))
      outfile.write("  { " + line[0] + ", " + ", ".join(quoted) + " },\n")

    count += 1

  string = "\n".join((
    "};",
    "// clang-format on",
    "",
    "}  // namespace stimulus",
    "",
    "#endif  // EXPERIMENTAL_GOOGLEX_AMBER_STIMULUS_V2_CATPREDITEMS_H_\n"))
  outfile.write(string)
  outfile.close()
