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

'use strict';

const settings = require('electron').remote.require('electron-settings');

class Config {
  static get KEYS() {
    // persisted settings
    return {
      PRESET: 'preset',
      SITE: 'site',
      TASK: 'task',
      PORT_NAME: 'portName',
      RECORDING_DIR: 'recordingDir',
      FILTER_ENABLED_LOWPASS: 'filterEnabledLowPass',
      FILTER_ENABLED_HIGHPASS: 'filterEnabledHighPass',
      FILTER_ORDER_LOWPASS: 'filterOrderLowPass',
      FILTER_ORDER_HIGHPASS: 'filterOrderHighPass',
      FILTER_LOWPASS: 'filterLowPass',
      FILTER_HIGHPASS: 'filterHighPass',
    };
  }
  static validKey(key) {
    if (!key) {
      return false;
    }
    return Object.values(Config.KEYS).indexOf(key) != -1;
  }
  constructor() {
    this.config = settings.getAll();
    console.log('config', this.config);
  }
  set(key, value) {
    if (!Config.validKey(key)) {
      console.log('config key invalid:', key);
      return false;
    }
    this.config[key] = value;
    settings.set(key, value);
    return value;
  }
  get(key) {
    if (!Config.validKey(key)) {
      console.log('config key invalid:', key);
      return false;
    }
    return this.config[key];
  }
}
