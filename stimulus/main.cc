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

#include <SDL.h>

#include <cstdint>
#include <set>
#include <vector>

#include "Calibration.h"
#include "Doors.h"
#include "EmotionalImages.h"
#include "EyesClosed.h"
#include "Flankers.h"
#include "HotButton.h"
#include "Image.h"
#include "LatencyTest.h"
#include "Mark.h"
#include "Platform.h"
#include "Random.h"
#include "Revision.h"
#include "Screen.h"
#include "Settings.h"
#include "Sret.h"
#include "Ssvep.h"
#include "WorkingMemory.h"
#include "Version.h"

namespace stimulus {
namespace {

const int kTextLeft = 200;
const int kTextTop = 200;
const int kDefaultBaudRate = 115200;
const char *kMarkSerialPortNameSetting = "mark_serialportname";
const char *kMarkParallelPortAddressSetting = "mark_parallelportaddress";

class TaskSelectionScreen : public Screen {
 public:
  TaskSelectionScreen() : version_string_(kFullVersionString) {}

  void IsActive() override {
    // We will come back here after finishing a task.
    CloseMarkFile();
  }

  void Render() override {
    int top = kTextTop;

    DrawString(kTextLeft, top, "Select Task:");
    top += GetFontHeight() * 2;

    for (auto label : labels_) {
      DrawString(kTextLeft, top, label);
      top += GetFontHeight();
    }

    const int bottom = GetDisplayHeightPx() - GetFontHeight();
    DrawString(0, bottom, version_string_);
  }

  void AddSelection(std::string name, Screen *screen) {
    task_names_.push_back(name);
    std::string label = std::to_string(labels_.size() + 1) + ". " + name;
    labels_.push_back(label);
    AddSuccessor(screen);
  }

  void KeyPressed(SDL_Scancode scancode) override {
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
      int screen_index = scancode - SDL_SCANCODE_1;
      if (screen_index < GetNumSuccessors()) {
        OpenMarkFile(task_names_[screen_index]);
        SwitchToScreen(screen_index);
      }
    }
  }

 private:
  std::vector<std::string> task_names_;
  std::vector<std::string> labels_;
  std::string version_string_;
};

class SerialSelectionScreen : public Screen {
 public:
  SerialSelectionScreen(std::vector<std::string> port_names, int baud_rate)
      : port_names_(port_names), baud_rate_(baud_rate) {}

  void Render() override {
    int top = kTextTop;
    int index = 1;

    DrawString(kTextLeft, top, "Select Serial Port:");
    top += GetFontHeight() * 2;
    for (auto port_name : port_names_) {
      DrawString(kTextLeft, top, std::to_string(index++) + ". " + port_name);
      top += GetFontHeight();
    }

    DrawString(kTextLeft, top, std::to_string(index) + ". None");
  }

  void KeyPressed(SDL_Scancode scancode) override {
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
      unsigned long port_index = scancode - SDL_SCANCODE_1;
      if (port_index == port_names_.size()) {
        // This is the 'none' setting.
        SwitchToScreen(0);
      } else if (port_index < port_names_.size()) {
        OpenMarkPort(port_names_[port_index], baud_rate_);
        SwitchToScreen(0);
      }
    }
  }

 private:
  std::vector<std::string> port_names_;
  int baud_rate_;
};

const std::set<int> kValidBaudRates{110,   300,    600,    1200,  2400,
                                    4800,  9600,   14400,  19200, 38400,
                                    57600, 115200, 120000, 256000};

bool CheckBaudRate(int rate) {
  return kValidBaudRates.find(rate) != kValidBaudRates.end();
}

}  // namespace
}  // namespace stimulus

int main(int argc, char *argv[]) {
  stimulus::InitRandom(stimulus::GetRandomSeed());
  stimulus::Settings settings(stimulus::GetResourceDir() + "/settings.txt");
  std::string errors = settings.GetErrors();
  if (errors.length() > 0) {
    std::string message = "There was a problem reading the settings file:\n";
    message += errors;
    stimulus::Screen::FatalError(message);
    return 1;
  }

  int baud_rate = stimulus::kDefaultBaudRate;
  if (settings.HasKey("baud_rate")) {
    baud_rate = settings.GetIntValue("baud_rate");
    if (!stimulus::CheckBaudRate(baud_rate)) {
      stimulus::Screen::FatalError(
          "Invalid baud rate specified in settings file");
      return 1;
    }
  }

  if (settings.HasKey("mark_format")) {
    std::string format = settings.GetValue("mark_format");
    if (format == "brainometer") {
      stimulus::SetMarkFormat(stimulus::kBrainometer);
    } else if (format == "byte") {
      stimulus::SetMarkFormat(stimulus::kByte);
    } else if (format == "parallelport") {
      stimulus::SetMarkFormat(stimulus::kParallel);
    } else {
      stimulus::Screen::FatalError(
          "Invalid mark format specified in settings file "
          "(must be 'brainometer', 'byte', or 'parallelport')");
      return 1;
    }
  }

  if (settings.HasKey("mark_directory")) {
    stimulus::SetMarkDirectory(settings.GetValue("mark_directory"));
  }

  if (!settings.HasKey("monitor_width") || !settings.HasKey("monitor_height")) {
    stimulus::Screen::FatalError(
        "missing monitor sizes in settings.txt. "
        "Ensure there are monitor_width and monitor_height keys (in cm).");
    return 1;
  }

  float width = settings.GetFloatValue("monitor_width");
  float height = settings.GetFloatValue("monitor_height");
  if (!stimulus::Screen::InitDisplay(width, height)) {
    return 1;
  }

  stimulus::TaskSelectionScreen *task_selection_screen =
      new stimulus::TaskSelectionScreen();
  stimulus::Screen *doors =
      stimulus::InitDoors(task_selection_screen, settings);
  task_selection_screen->AddSelection("Doors", doors);
  stimulus::Screen *emotional_images =
      stimulus::InitEmotionalImages(task_selection_screen, settings);
  task_selection_screen->AddSelection("Emotional Images", emotional_images);
  stimulus::Screen *flankers =
      stimulus::InitFlankers(task_selection_screen, settings);
  task_selection_screen->AddSelection("Flankers", flankers);
  stimulus::Screen *hb =
      stimulus::InitHotButton(task_selection_screen, settings);
  task_selection_screen->AddSelection("Hot Button", hb);
  stimulus::Screen *sret = stimulus::InitSret(task_selection_screen, settings);
  task_selection_screen->AddSelection("SRET", sret);
  stimulus::Screen *ssvep =
      stimulus::InitSsvep(task_selection_screen, settings);
  task_selection_screen->AddSelection("SSVEP", ssvep);
  stimulus::Screen *wm =
      stimulus::InitWorkingMemory(task_selection_screen, settings);
  task_selection_screen->AddSelection("Working Memory", wm);
  stimulus::Screen *eyes_closed =
      stimulus::InitEyesClosed(task_selection_screen);
  task_selection_screen->AddSelection("Eyes Closed", eyes_closed);
  stimulus::Screen *calibration =
      stimulus::InitCalibration(task_selection_screen);
  task_selection_screen->AddSelection("Oddball", calibration);
  stimulus::Screen *latency_test =
      stimulus::InitLatencyTest(task_selection_screen);
  task_selection_screen->AddSelection("Latency Test", latency_test);

  if (settings.HasKey(stimulus::kMarkParallelPortAddressSetting) &&
      settings.HasKey(stimulus::kMarkSerialPortNameSetting)) {
    stimulus::Screen::FatalError(
        "invalid configuration: both serial and parallel port specified");
    return 1;
  }

  stimulus::Screen *first_screen = task_selection_screen;
  if (settings.HasKey(stimulus::kMarkSerialPortNameSetting)) {
    stimulus::OpenMarkPort(
        settings.GetValue(stimulus::kMarkSerialPortNameSetting), baud_rate);
  } else if (settings.HasKey(stimulus::kMarkParallelPortAddressSetting)) {
    stimulus::OpenMarkPort(
        settings.GetValue(stimulus::kMarkParallelPortAddressSetting), 0);
  } else {
    // prompt for port selection if not specified in settings
    std::vector<std::string> ports = stimulus::GetAvailableSerialPorts();
    stimulus::SerialSelectionScreen *serial_selection_screen =
        new stimulus::SerialSelectionScreen(ports, baud_rate);
    serial_selection_screen->AddSuccessor(task_selection_screen);
    first_screen = serial_selection_screen;
  }

  stimulus::Screen::MainLoop(first_screen);

  return 0;
}
