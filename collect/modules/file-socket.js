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

'use strict'

const DataSocket = require('./data-socket.js');
const fs = require('fs');
const Readline = require('readline');

module.exports = class FileSocket extends DataSocket {
  constructor(connectCb, disconnectCb, dataCb, file) {
    super('file-socket', connectCb, disconnectCb, dataCb);
    this.LINE_DELAY_MS = 4;
    this._file = file;
    this._lines = [];
  }

  setName(name) {
    // Set name is called for the first time when the front-end is
    // ready to accept data.
    if (name != this._file) {
      return;
    }
    // Below should only be called once.
    if (name != this._name) {
      super.setName(name);

      let stream = fs.createReadStream(name);
      stream.on('ready', function() {
        this._readline = Readline.createInterface({input: stream});

        this.debug('start');

        this._connectCb('channelData');
        this._connectCb('config');
        this._connectCb('impedance');

        this._readline.on('line', function(line) {
          // skip the header
          if ((line[0] != '{') && (line[0] != '-')) {
            this._lines.push(line);
          }
        }.bind(this));

        // output a line with a delay to mimic serial data
        setTimeout(this._doLine.bind(this), this.LINE_DELAY_MS);

        this._readline.on('close', function() {
          this._disconnectCb('channelData');
          this._disconnectCb('config');
          this._disconnectCb('impedance');
        }.bind(this));
      }.bind(this));
    }
  }

  listNames() {
    return [this._file];
  }

  _doLine() {
    if (this._lines.length > 0) {
      // put the newline back because boxer relies on it for parsing
      this._dataCb('channelData', this._lines.shift() + '\r\n');
    }
    setTimeout(this._doLine.bind(this), this.LINE_DELAY_MS);
  }
}
