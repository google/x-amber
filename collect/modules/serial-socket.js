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

serial-socket.js

**********************************************************************/


'use strict'

const DataSocket = require('./data-socket.js');
const SerialPort = require('serialport');
const Registry = require('winreg');

module.exports = class SerialSocket extends DataSocket {
  constructor(connectCb, disconnectCb, dataCb) {
    if (!connectCb || !disconnectCb || !dataCb) {
      this.debug('ERROR: all callbacks must be set');
    }

    super('serial-socket', connectCb, disconnectCb, dataCb);

    this.DELAY_BETWEEN_CONNECT_ATTEMPTS = 10000;
    this.DELAY_BEFORE_CHECKING_IF_CONNECTED = 2000;

    this._allPorts = [];
    this._currentPort = null;
    this._sp = null;
  }

  init(updateClientSerialPorts) {
    // poll for new serial ports
    this._pollPorts(updateClientSerialPorts);
  }

  setName(name) {
    let port = this._allPorts.find(p => p.comName == name);
    if (!port) {
      this.debug('ERROR: invalid port', name);
      return;
    }
    if (name != this._name) {
      super.setName(name);
      this._currentPort = port;
      this._connect();
    }
  }

  // lists the ports from the last polling cycle
  listNames() {
    return this._allPorts.reduce((acc, curr) => acc.concat(curr.comName), []);
  }

  send(cmd) {
    if (this._sp == null) {
      this.debug('Cannot send to serial without connection.');
      return;
    }
    this._sp.write(cmd);
  }

  _connect() {
    if (!this._currentPort) {
      this.debug('ERROR: no port selected');
      return;
    }
    if (this._sp != null && this._sp.isOpen) {
      this._sp.close(function(err) {
        this.debug('ERROR: error on closing serial port:', err);
      }.bind(this));
      this._sp = null;
    }

    // do platform-specific setup before opening the port
    this._setup(function(err) {
      if (err) {
        this.debug('Error setting up port: ', err);
        return;
      }

      // now open the port
      this._sp = new SerialPort(
          this._currentPort.comName, {baudRate: 921600}, function(err) {
            if (err) {
              this.debug('Error opening port: ', err.message);
            } else {
              this._isConnected = true;

              this.debug('connected to', this._currentPort.comName);

              this._parser.on('data', this._fullLineHandler.bind(this));

              this._connectCb('channelData');
              this._connectCb('config');
              this._connectCb('impedance');
            }
          }.bind(this));

      this._sp.on('close', function() {
        this.debug('Serial port disconnect!');
        this._disconnect();
        this._disconnectCb('channelData');
        this._disconnectCb('config');
        this._disconnectCb('impedance');
      }.bind(this));

      // Readline parser.
      this._parser =
          this._sp.pipe(new SerialPort.parsers.Readline({delimiter: '\n'}));
    }.bind(this));
  }

  _disconnect() {
    this._isConnected = false;

    this.clearName();
    this._parser = null;
    this._sp = null;
  }

  _setup(cb) {
    if (process.platform != 'win32') {
      // nothing special to do
      cb(null);
      return;
    }

    // apply the serial port latency setting
    const Ftdi = require('./ftdi-win.js');
    new Ftdi().setLatency(this._currentPort.pnpId, 2, cb);
  }

  // poll for new connection ports every 2 sec and update web client
  _pollPorts(updateClientSerialPorts) {
    SerialPort.list().then(function(_ports) {
      // only show ftdi chips (used in bioamp)
      this._allPorts =
          _ports.filter(p => p.vendorId == '0403' && p.productId == '6001');
      let serialPorts =
          this._allPorts.reduce((acc, curr) => acc.concat(curr.comName), [])
      updateClientSerialPorts(serialPorts);

      setTimeout(function() {
        this._pollPorts(updateClientSerialPorts);
      }.bind(this), 2 * 1000);
    }.bind(this));
  }
}
