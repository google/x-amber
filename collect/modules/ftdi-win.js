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

const Registry = require('winreg');
const sudo = require('sudo-prompt');
const path = require('path');

// FIXME this exec's reg.exe a bunch of times to read the registry
// and elevates privileges using UAC to write to the registry. It's
// probably better to make a native binding to perform registry operations.

module.exports = class FtdiWindows {

  constructor() {
    this.debug = require('debug')('collect:ftdi-win');
  }

  _registryGetValue(key, name) {
    return new Promise((resolve, reject) => {
      key.get(name, (_err, _item) => {
        if (_err) {
          reject('registry get error ' + _err + ' for key \'' + key.key +
            '\' name=\'' + name + '\'');
        } else {
          resolve(_item.value);
        }
      });
    });
  }

  /**
   * @param key string
   * @param latency_ms number
   */
  _registrySetLatency(key, latency_ms) {
    return new Promise((resolve, reject) => {
      let cmd = [
        path.join(process.env.windir, 'system32', 'reg.exe'),
        'ADD',
        '"HKLM' + key + '"',
        '/v LatencyTimer',
        '/t REG_DWORD',
        '/d', latency_ms,
        '/f'
      ];
      sudo.exec(cmd.join(' '), {name: 'reg'}, (err, stdout, stderr) => {
        if (err) {
          reject('registry set error ' + err);
        } else {
          resolve();
        }
      });
    });
  }

  /**
   * @param pnpId string
   * @param latency_ms number
   * @param cb function
   */
  async setLatency(pnpId, latency_ms, cb) {
    try {
      // find the registry key corresponding to this port
      const key = new Registry({
        hive: Registry.HKLM,
        key: '\\SYSTEM\\CurrentControlSet\\Enum\\' + pnpId + '\\Device Parameters',
      });
      // check the latency setting
      let current = await this._registryGetValue(key, 'LatencyTimer');
      // set the latency if needed
      if (current != latency_ms) {
        await this._registrySetLatency(key.key, latency_ms);
      }
      cb(null);
    } catch (err) {
      cb(err);
    }
  }
};
