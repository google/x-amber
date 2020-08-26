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

cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/third_party/cmake/")
project(Stimulus)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(JPEG REQUIRED)

# Boost is only used for the unit test framework. It should not be linked into the
# main applications (since we don't have libraries for Windows)
find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS} ${BOOST_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_STANDARD 11)

add_executable(stimulus
  main.cc
  Calibration.cc
  Doors.cc
  EmotionalImages.cc
  EyesClosed.cc
  Flankers.cc
  FlankersEngine.cc
  HotButton.cc
  HotButtonEngine.cc
  Image.cc
  LatencyTest.cc
  Mark.cc
  PlatformPosix.cc
  Random.cc
  Screen.cc
  Settings.cc
  Sret.cc
  Ssvep.cc
  Util.cc
  WorkingMemory.cc)

target_link_libraries(stimulus ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${JPEG_LIBRARIES})
if(NOT MSVC)
  target_compile_options(stimulus PRIVATE -Wall -W -Wno-unused-parameter)
endif()

add_custom_target(versionfile
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_git_version.sh ${CMAKE_CURRENT_BINARY_DIR}/Revision.h)
add_dependencies(stimulus versionfile)

add_executable(random_sequence_test
  random_sequence.cc
  Random.cc)

foreach(file ${RESOURCE_FILES})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} COPYONLY)
endforeach()

set(TEST_DATA_FILES
  test-settings-good.txt
  test-settings-missing-value.txt)

foreach(file ${TEST_DATA_FILES})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} COPYONLY)
endforeach()

add_custom_command(
  TARGET stimulus PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:stimulus>/resources)

add_executable(unit_tests
  UnitTestMain.cc
  SettingsTest.cc
  Settings.cc
  ShufflerTest.cc
  RandomTest.cc
  Random.cc
)

target_link_libraries(unit_tests ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
