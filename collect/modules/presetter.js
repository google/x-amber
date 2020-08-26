/**
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

/**********************************************************************

presetter.js

This object manages the reading/writing of preset files.
Presets are effectively lookup tables of various settings
that the visualizer references
e.g. sample rate, channels that are on/off, channel labels, etc.
Presets are stored in the "presets" folder as json files.

**********************************************************************/


'use strict'

module.exports = class Presetter {
  constructor(top) {
    this.top = top;
    this.debug = require('debug')('collect:presetter');
    this.PRESETS_FOLDER_PATH = 'presets/';
    this.presets = [];
    this.fs = require('fs');
    this.path = require('path');
    this.loadPresets();
  }


  ////////////////////////////////////////////////
  // accessors
  ////////////////////////////////////////////////

  getPresets() {
    return this.presets;
  }

  getPreset(presetNameToFind) {
    for (var i = 0; i < this.presets.length; i++) {
      var preset = this.presets[i];
      var presetName = preset['name'];
      if (presetName == presetNameToFind) {
        return preset;
      }
    }
    return null;
  }

  setPreset(presetNameToFind, newPreset) {
    for (var i = 0; i < this.getPresets().length; i++) {
      var preset = this.getPresets()[i];
      var presetName = preset['name'];
      if (presetName == presetNameToFind) {
        this.getPresets()[i] = newPreset;
        return;
      }
    }
    // if we made it here, then the preset doesn't exist
    // so we add it now
    this.getPresets().push(newPreset);
  }


  ////////////////////////////////////////////////
  // utilities
  ////////////////////////////////////////////////

  load() {
    loadPresets();
  }

  // Loop through all the json files in the presets folder
  // and extract each presets data
  loadPresets() {
    var fileNames = this.fs.readdirSync(this.PRESETS_FOLDER_PATH);
    for (var i = 0; i < fileNames.length; i++) {
      var fileName = fileNames[i];
      var fileExtension = this.path.extname(fileName)
      if (fileExtension == '.json') {
        var presetFilePath =
            this.path.resolve(this.PRESETS_FOLDER_PATH + fileName);
        var presetJson = this.fs.readFileSync(presetFilePath);
        var preset = JSON.parse(presetJson);
        for (let i = 0; i < preset.channelData.length; i++) {
          if (preset.channelData[i].index == undefined) {
            preset.channelData[i].index = i;
          }
        }
        this.presets.push(preset);
      }
    }
  }

  // Save the given preset to file.
  // It wholesale overwrites the existing file.
  savePreset(preset) {
    var presetName = preset['name'];
    this.debug('savePreset', presetName);
    this.setPreset(presetName, preset);
    var presetJson = JSON.stringify(preset);
    var presetFilePath = this.path.resolve(
        this.PRESETS_FOLDER_PATH + presetName + '_preset.json');
    this.fs.writeFileSync(presetFilePath, presetJson);
    this.top.getServerer().sendPresetSavedMessageToWebClient();
  }
}
