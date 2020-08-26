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

#include <windows.h>
#include "Platform.h"
#include "Screen.h"
#include <chrono>
#include <thread>

namespace stimulus {
namespace {
HANDLE serial_port = INVALID_HANDLE_VALUE;
HANDLE read_event = INVALID_HANDLE_VALUE;
HANDLE write_event = INVALID_HANDLE_VALUE;

typedef void (_stdcall *oupfuncPtr) (short portaddr, unsigned short datum);
HINSTANCE hLib;
oupfuncPtr out;
int parallelportNumber;

void PrintSyscallError(const char *function, const char *call) {
  char message_buffer[256];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0,
                message_buffer, sizeof(message_buffer), NULL);
  Screen::FatalError(std::string("Error: ") + function + ":" + call + ":"
      + message_buffer);
}

}  // namespace

int OpenParallel(const std::string &portName) {
  sscanf (portName.c_str(), "%x", &parallelportNumber); // convert hex string to int
  // load parallel port DLL
  hLib = LoadLibrary("inpout32.dll");
  if (hLib != NULL) {
    out = (oupfuncPtr)GetProcAddress(hLib, "Out32");
    if (out != NULL) {
      out(parallelportNumber, 0);   //set all pins low
    } else {
      Screen::FatalError("Unable to send data over specified port address");
      return -1;
    }
  } else {
    Screen::FatalError("Inpout32.dll unable to be loaded");
    return -1;
  }
  return 0;
}

void WriteParallel(int datum) {
  out(parallelportNumber, datum);   //send code to data port pin
  std::this_thread::sleep_for(std::chrono::milliseconds(2)); // pulse duration
  out(parallelportNumber, 0);   //set all pins low
}

int OpenSerial(const std::string &name, int baud_rate) {
  serial_port = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE,
                          0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
  if (serial_port == INVALID_HANDLE_VALUE) {
    PrintSyscallError(__FUNCTION__, "CreateFile");
    return -1;
  }

  DCB port_state;
  memset(&port_state, 0, sizeof(port_state));
  port_state.DCBlength = sizeof(DCB);

  char def_string[256];
  snprintf(def_string, sizeof(def_string),
      "baud=%d parity=N data=8 stop=1 xon=off odsr=off octs=off dtr=off "
      "rts=off idsr=off",
      baud_rate);
  if (!BuildCommDCBA(def_string, &port_state)) {
    PrintSyscallError(__FUNCTION__, "BuildCommDCBA");
    CloseHandle(serial_port);
    return -1;
  }

  if (!SetCommState(serial_port, &port_state)) {
    PrintSyscallError(__FUNCTION__, "SetCommState");
    CloseHandle(serial_port);
    return -1;
  }

  COMMTIMEOUTS timeouts;
  memset(&timeouts, 0, sizeof(timeouts));
  timeouts.ReadIntervalTimeout = 1;
  if (!SetCommTimeouts(serial_port, &timeouts)) {
    PrintSyscallError(__FUNCTION__, "SetCommTimeouts");
    CloseHandle(serial_port);
    return -1;
  }

  PurgeComm(serial_port, PURGE_RXCLEAR);
  PurgeComm(serial_port, PURGE_TXCLEAR);

  read_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (read_event == INVALID_HANDLE_VALUE) {
    PrintSyscallError(__FUNCTION__, "CreateEvent [read_event]");
    return -1;
  }

  write_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (write_event == INVALID_HANDLE_VALUE) {
      PrintSyscallError(__FUNCTION__, "CreateEvent [write_event]");
      return -1;
  }

  return 0;
}

void CloseSerial() {
  CloseHandle(serial_port);
}

int WriteSerial(const void *buf, int length) {
  OVERLAPPED overlap;
  overlap.hEvent = write_event;
  overlap.Offset = 0;
  overlap.OffsetHigh = 0;

  if (!WriteFile(serial_port, buf, length, NULL, &overlap)
      && GetLastError() != ERROR_IO_PENDING) {
    PrintSyscallError(__FUNCTION__, "WriteFile");
    return -1;
  }

  DWORD bytes_transferred;
  if (!GetOverlappedResult(serial_port, &overlap, &bytes_transferred, TRUE)) {
    PrintSyscallError(__FUNCTION__, "GetOverlappedResult");
    return -1;
  }

  return bytes_transferred;
}

int ReadSerial(void *buf, int length) {
  OVERLAPPED overlap;
  overlap.hEvent = read_event;
  overlap.Offset = 0;
  overlap.OffsetHigh = 0;

  if (!ReadFile(serial_port, buf, length, NULL, &overlap)
      && GetLastError() != ERROR_IO_PENDING) {
    PrintSyscallError(__FUNCTION__, "ReadFile");
    return -1;
  }

  DWORD bytes_transferred;
  if (!GetOverlappedResult(serial_port, &overlap, &bytes_transferred, TRUE)) {
    PrintSyscallError(__FUNCTION__, "GetOverlappedResult");
    return -1;
  }

  return bytes_transferred;
}

std::vector<std::string> GetAvailableSerialPorts() {
  std::vector<std::string> ports;
  for (int i = 0; i < 256; i++) {
    std::string name;
    if (i > 9) {
      // This is a windows quirk.
      // https://support.microsoft.com/en-us/help/115831/howto-specify-serial-ports-larger-than-com9
      name = "\\\\.\\COM";
    } else {
      name = "COM";
    }

    name += std::to_string(i);
    HANDLE serial_port = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE,
        0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (serial_port == INVALID_HANDLE_VALUE) {
      continue;
    }

    DCB port_state;
    port_state.DCBlength = sizeof(DCB);
    if (!GetCommState(serial_port, &port_state)) {
        CloseHandle(serial_port);
        continue;
    }

    CloseHandle(serial_port);
    ports.push_back(name);
  }

  return ports;
}

std::string GetResourceDir() { return ".\\resources\\"; }

uint32_t GetRandomSeed() {
  return GetTickCount();
}

DateTime GetDateTime() {
  SYSTEMTIME time;
  DateTime retval;

  GetLocalTime(&time);

  retval.year = time.wYear;
  retval.month = time.wMonth;
  retval.day = time.wDay;
  retval.hour = time.wHour;
  retval.minute = time.wMinute;
  retval.second = time.wSecond;

  return retval;
}

}  // namespace stimulus
