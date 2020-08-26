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

module.exports = class DataSocket {
  constructor(tag, connectCb, disconnectCb, dataCb) {
    this._isConnected = false;
    this._name = '';
    this._connectCb = connectCb;
    this._disconnectCb = disconnectCb;
    this._dataCb = dataCb;

    this.debug = require('debug')('collect:' + tag);
  }

  init(updateClientSerialPorts) {}

  getIsConnected() {
    return this._isConnected;
  }

  setName(name) {
    this._name = name;
  }

  clearName() {
    this._name = '';
  }

  listNames() {
    return [];
  }

  send(cmd) {}

  _fullLineHandler(line) {
    if (!this._dataCb) {
      this.debug('ERROR: got data, but no data callback registered:', line);
      return;
    }

    let len = line.length;
    // Expect prefixed data with at least 3 characters and a colon.
    if (len <= 4) {
      this.debug('ERROR: data too short:', line);
      return;
    }

    // Data Ingestion shortcut
    if (line.charAt(1) == 'D' && line.charAt(2) == 'A' &&
        line.charAt(3) == 'T' && line.charAt(4) == 'A' &&
        line.charAt(5) == ':') {
      this._dataCb('channelData', line.substring(6) + '\r\n');
      return;
    }

    // Normal Process
    let trimmed = line.trim();
    let i = trimmed.indexOf(':');
    if (i == -1) {
      this.debug('ERROR: malformed data:', line);
    } else {
      let split = [
        trimmed.substring(0, i),
        trimmed.substring(i + 1),
      ];
      if (split[0] == 'DATA') {
        this._dataCb('channelData', split[1] + '\r\n');
      } else if (split[0] == 'CLI') {
        this._dataCb('config', split[1]);
      } else if (split[0] == 'IMP') {
        this._dataCb('impedance', split[1]);
      } else {
        this.debug('ERROR: malformed data:', line);
      }
    }
  }
};
