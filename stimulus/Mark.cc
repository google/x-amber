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

#include <fstream>
#include <cstdio>
#include "Platform.h"
#include "Screen.h"
#include "Mark.h"
#include "Version.h"

namespace stimulus {
namespace {

bool serial_port_open;
MarkFormat mark_format = kBrainometer;
std::string mark_directory;
std::string mark_task;
std::vector<std::string> mark_tracking_vector;

}  // namespace

void SetMarkFormat(MarkFormat format) {
  mark_format = format;
}

void SendMark(int num, const std::string &event) {
  Uint32 now = SDL_GetTicks();

  if (serial_port_open) {
    switch (mark_format) {
      case kBrainometer: {
        char tmp[32];
        int len = snprintf(tmp, sizeof(tmp), "mark %d\r\n", num);
        WriteSerial(tmp, len);
        break;
      }

      case kByte: {
        char val = num & 0xff;
        WriteSerial(&val, 1);
        break;
      }

      case kParallel: {
        WriteParallel(num); //send code to data port pin
        break;
      }
    }
  }

  SDL_Log("mark %d\n", num);

  // log trigger, onset, stimulus
  mark_tracking_vector.push_back(std::to_string(num) + ',' + std::to_string(now) + ',' + event + "\r\n");
}

void OpenMarkPort(const std::string &portName, int baudRate) {
  if ((mark_format == kBrainometer) || (mark_format == kByte)) {
    if (OpenSerial(portName, baudRate) >= 0) {
      serial_port_open = true;
    } else {
      Screen::FatalError("Error opening serial port");
    }
  } else if (mark_format == kParallel) {
    if (OpenParallel(portName) >= 0) {
      serial_port_open = true;
    } else {
      Screen::FatalError("Error opening parallel port");
    }
  }
}

void SetMarkDirectory(const std::string &dir) {
  mark_directory = dir;
}

void OpenMarkFile(const std::string &task) {
  mark_tracking_vector.clear();
  mark_task = task;
}

void CloseMarkFile() {
  if (!mark_tracking_vector.empty() && !mark_directory.empty()) {
    DateTime when = GetDateTime();

    // Note: can't use colon in the date string because it isn't a valid
    // filename character on Windows.
    char date_string[256];
    snprintf(date_string, sizeof(date_string), "%d-%d-%d_%02d-%02d-%02d",
      when.month, when.day, when.year, when.hour, when.minute, when.second);
    std::string path = mark_directory + mark_task + "_" + date_string + ".csv";
    SDL_Log("Writing marks to %s\n", path.c_str());
    std::ofstream mark_file(path);
    if (!mark_file) {
      Screen::FatalError("Couldn't open mark output file. Ensure directory in settings file exists.");
      return;
    }

    mark_file << "{\r\n";
    mark_file << "  \"gentask\": \"River2\",\r\n"; // this allows read-in functions to change format if necessary
    mark_file << "  \"file_type\": \"mark\",\r\n";
    mark_file << "  \"date\": \"" << date_string << "\",\r\n";
    mark_file << "  \"task\": \"" << mark_task << "\",\r\n";
    mark_file << "  \"version\": \"" << kFullVersionString << "\"\r\n";
    mark_file << "}\r\n----\r\n";

    mark_file << "Type,Time,Event\r\n";
    for (const auto &mark_string : mark_tracking_vector) {
      mark_file << mark_string;
    }

    if (!mark_file) {
        Screen::FatalError("Error writing to mark file.");
        return;
    }

    mark_file.close();
  }

  mark_tracking_vector.clear();
}

}  // namespace stimulus
