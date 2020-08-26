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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include "Platform.h"
#include "Screen.h"

namespace stimulus {
namespace {
int serial_fd;
bool have_resource_dir;
std::string resource_dir;

void PrintSyscallError(const std::string &function, const std::string &call) {
  Screen::FatalError(function + ": " + call + ": " + strerror(errno));
}

// An entry in the /sys/class/tty/*/dev directory contains the ASCII string:
// major:minor. Parse that into an integer device ID, suitable to compare
// to struct stat -> st_dev.
int ParseDeviceId(const char *device_string) {
  const char *colon = strchr(device_string, ':');
  if (colon == nullptr) {
    return -1;
  }

  int major = strtol(device_string, nullptr, 10);
  int minor = strtol(colon + 1, nullptr, 10);
  return makedev(major, minor);
}

std::map<int, int> kBaudMap = {
  { 300, B300 },
  { 600, B600 },
  { 1200, B1200 },
  { 1800, B1800 },
  { 2400, B2400 },
  { 4800, B4800 },
  { 9600, B9600 },
  { 19200, B19200 },
  { 38400, B38400 },
  { 57600, B57600 },
  { 115200, B115200 },
  { 230400, B230400 },
#ifndef __APPLE__
  { 460800, B460800 },
  { 921600, B921600 }
#endif
};

bool IsTty(const std::string &path) {
  int fd = open(path.c_str(), O_RDWR);
  if (fd < 0) {
    return false;
  }

  bool result = isatty(fd);
  close(fd);

  return result;
}

}  // namespace

int OpenParallel(const std::string &portName) {
  Screen::FatalError("Open parallel port not supported on Posix");
  return -1;
}

void WriteParallel(int datum) {
  Screen::FatalError("Write parallel port not supported on Posix");
}

int OpenSerial(const std::string &name, int baud_rate) {
  struct termios serial_opts;

  std::string path = "/dev/";
  path += name;
  serial_fd = open(path.c_str(), O_RDWR | O_NOCTTY);
  if (serial_fd < 0) {
    PrintSyscallError(__FUNCTION__, "open");
    return -1;
  }

  memset(&serial_opts, 0, sizeof(serial_opts));

  serial_opts.c_iflag = IGNBRK;
  serial_opts.c_cflag = CLOCAL | CREAD | CS8;
  auto speed_constant = kBaudMap.find(baud_rate);
  if (speed_constant == kBaudMap.end()) {
    Screen::FatalError("Invalid baud rate specified");
    close(serial_fd);
    return -1;
  }

  cfsetispeed(&serial_opts, speed_constant->second);
  cfsetospeed(&serial_opts, speed_constant->second);
  serial_opts.c_cc[VMIN] = 1;

  if (tcsetattr(serial_fd, TCSANOW, &serial_opts) != 0) {
    PrintSyscallError(__FUNCTION__,  "tcsetattr");
    close(serial_fd);
    return -1;
  }

  tcflush(serial_fd, TCIOFLUSH);
  return 0;
}

void CloseSerial() {
  close(serial_fd);
}

int WriteSerial(const void *buf, int length) {
  return write(serial_fd, buf, length);
}

int ReadSerial(void *buf, int length) {
  return read(serial_fd, buf, length);
}

std::string GetResourceDir() {
#ifdef __linux__
  if (!have_resource_dir) {
    have_resource_dir = true;
    char exe_path[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", exe_path, sizeof(exe_path));
    if (length < 0) {
      PrintSyscallError(__FUNCTION__, "readlink");
      return "";
    }

    std::string path(exe_path, length);
    size_t index = path.rfind('/');
    path = path.substr(0, index + 1);

    if (path.find("blaze-out") != std::string::npos) {
      // XXX hack: check if I was run by blaze
      path += "resources/";
    }

    resource_dir = path + "/resources/";
    SDL_Log("resource dir is %s\n", resource_dir.c_str());
  }

  return resource_dir;
#else
  char path[PATH_MAX];
  getcwd(path, sizeof(path));
  return std::string(path) + "/resources/";
#endif
}

std::vector<std::string> GetAvailableSerialPorts() {
  std::vector<std::string> ports;

#ifdef __linux__
  // First scan the TTY directory. All serial ports should be here.
  const std::string sys_path = "/sys/class/tty";
  std::set<int> devices;
  DIR *sys_dir = opendir(sys_path.c_str());
  while (true) {
    struct dirent *entry = readdir(sys_dir);
    if (entry == nullptr) {
      break;
    }

    std::string entry_path = sys_path + "/" + entry->d_name;

    // Pseudoterminals will not have device directories. Skip them.
    std::string device_path = entry_path + "/device";
    if (access(device_path.c_str(), F_OK) < 0) {
      continue;
    }

    // This is probably a serial device, save the device number.
    std::string dev_path = entry_path + "/dev";
    FILE *file = fopen(dev_path.c_str(), "r");
    char contents[63];
    if (fread(contents, 1, sizeof(contents), file) > 0) {
      devices.insert(ParseDeviceId(contents));
    }

    fclose(file);
  }

  closedir(sys_dir);

  // Now need to scan the /dev directory to look for entries that have the same
  // device number as the entries we found above.
  const std::string dev_path = "/dev";
  DIR *dev_dir = opendir(dev_path.c_str());
  while (true) {
    struct dirent *entry = readdir(dev_dir);
    if (entry == nullptr) {
      break;
    }

    std::string full_path = dev_path + "/" + entry->d_name;
    struct stat st;
    if (stat(full_path.c_str(), &st) < 0) {
      continue;
    }

    if (devices.find(st.st_rdev) == devices.end()) {
      continue;
    }

    if (!IsTty(full_path)) {
      continue;
    }

    ports.push_back(entry->d_name);
  }

  closedir(dev_dir);
#endif

  return ports;
}

uint32_t GetRandomSeed() {
  return time(NULL);
}

DateTime GetDateTime() {
  DateTime retval;
  time_t timeval;
  struct tm curtime;

  timeval = time(nullptr);
  localtime_r(&timeval, &curtime);

  retval.year = curtime.tm_year + 1900;
  retval.month = curtime.tm_mon + 1;
  retval.day = curtime.tm_mday;
  retval.hour = curtime.tm_hour;
  retval.minute = curtime.tm_min;
  retval.second = curtime.tm_sec;

  return retval;
}

}  // namespace stimulus
