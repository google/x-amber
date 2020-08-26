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

boxer.js

This is the interface to the luchador box hardware.
All communication to and from the box happens over serial port.
Lastly, it also manages recording channel sensor data to files.

**********************************************************************/


'use strict'

const path = require('path');

var SerialSocket = require('./serial-socket.js');
var FileSocket = require('./file-socket.js');

module.exports = class Boxer {
  constructor(top, file = '') {
    this.top = top;
    this.debug = require('debug')('collect:boxer');
    this.fs = require('fs');

    this.amRecording = false;

    if (file) {
      this.debug('using file: ', file);
      this.socket = new FileSocket(
        this.onSocketConnect.bind(this), this.onSocketDisconnect.bind(this),
        this.onSocketData.bind(this), file);
    } else {
      this.socket = new SerialSocket(
          this.onSocketConnect.bind(this), this.onSocketDisconnect.bind(this),
          this.onSocketData.bind(this));
    }

    this.commandQueue = [];
    this.pendingCommand = null;

    this.previouslyRecordedDataString = '';

    this.socket.init(this.updateClientSerialPorts.bind(this));
  }


  ////////////////////////////////////////////////
  // accessors
  ////////////////////////////////////////////////

  getChannelDataSocket() {
    return this.socket;
  }
  getAmRecording() {
    return this.amRecording;
  }
  setAmRecording(newValue) {
    this.amRecording = newValue;
  }
  getSerialPortNames() {
    return this.socket.listNames();
  }


  ////////////////////////////////////////////////
  // callbacks
  ////////////////////////////////////////////////

  // Fires when the given socket makes a connection
  onSocketConnect(socketName) {
    if (socketName == 'channelData') {
      // Tell the web client about it
      this.top.getServerer().sendChannelDataSocketConnectedMessageToWebClient();
    } else if (socketName == 'config') {
      this.queueCommand('ver');
    }
  }

  // Fires when the given socket disconnects
  onSocketDisconnect(socketName) {
    // If the socket that closed is the channel data socket
    if (socketName == 'channelData') {
      // Tell the web client about it
      this.top.getServerer().sendChannelDataSocketClosedMessageToWebClient();
    }
  }

  // Fires when the given socket receives data
  onSocketData(socketName, data) {
    // If the data is from the channel data socket
    if (socketName == 'channelData') {
      // do some basic data format checks
      data = data.trim();
      if (!data.match(/^[0-9\-\,]+$/)) {
        this.debug('ERROR: malformed data: ' + data);
        return;
      }
      // older data has an extra column
      let split = data.split(',');
      if ((split.length != 34) && (split.length != 35)) {
        this.debug('ERROR: malformed data: ' + data);
        return;
      }
      // Write to the file if recording
      if (this.getAmRecording() == true) {
        this.recordChannelDataToStream(data);
      }

      // Send the data to the web client
      this.top.getServerer().sendChannelDataStringMessageToWebClient(data);

      // Pass the data to the artifacter
      this.top.getArtifacter().receiveData(data);

      // If the data is from the config box socket
    } else if (socketName == 'config') {
      this.debug('receive ', data.toString());
      if (this.pendingCommand == 'ver') {
        this.handleVersionResponse(data.toString());
      }

      // The FW protocol doesn't offer a straightforward way to match
      // command/response so just throw this away...
      this.pendingCommand = null;

      // Send the next command
      this.sendNextCommand();

      // // If the data is from the impedance data socket
    } else if (socketName == 'impedance') {
      // Send the impedance data to the web client
      this.top.getServerer().sendImpedanceDataStringToWebClient(
          data.toString());
    }
  }

  // Fires when the given socket encounters an error
  onSocketError(socketName) {
    // If the socket that encountered the error is the channel data socket
    if (socketName == 'channelData') {
      // Tell the web client about it
      this.top.getServerer().sendChannelDataSocketErrorMessageToWebClient()
    }
  }

  // Fires when checking if the given socket is connected
  onSocketConnectionCheck(socketName, isConnected) {
    // Nothing to do for channelData and impedance (unused) sockets.
    if (socketName != 'config') {
      return;
    }
    // Notify front end of failure
    if (isConnected == false) {
      this.top.getServerer()
          .sendUnableToConnectToChannelDataSocketMessageToWebClient();
    }
  }

  // Sends ports to web client when polling
  updateClientSerialPorts(serialPorts) {
    var serialPortsString = JSON.stringify(serialPorts);
    var message = {
      'serialPorts': serialPortsString,
    };
    this.top.getServerer().sendSerialPortsMessageToWebClient(message);
  }

  // Fires when the web client requests to connect
  // to the luchador box at the given ip
  onConnectToBoxMessage(portName) {
    this.debug('onConnectToBoxMessage port', portName);
    // Store the given ip or serial address.
    this.socket.setName(portName);
  }

  // Sends a message indicating a mark occurred to
  // the board.
  sendMarkMessage(value) {
    var commandString = 'mark ' + value.toString();
    this.queueCommand(commandString);
  }

  // Fires when the web client requests to
  // turn the given channel on or off
  onTurnChannelOnOrOffMessage(channelIndex, isOn) {
    // Python considers the channels as one-based not zero-based so we increment
    // the index here
    channelIndex += 1;
    // Python wants a 1 or 0 instead of true or false so we convert here
    var isOnString = '1';
    if (isOn == false) {
      isOnString = '0';
    }
    // Build the config command string that will be sent to the luchador box and
    // send it
    var commandString =
        'chon ' + channelIndex.toString() + ',' + isOnString;
    this.queueCommand(commandString);
  }

  // Fires when the web client wants to send
  // a given mark.
  onSendMarkToAmplifier(mark) {
    this.sendMarkMessage(mark);
  }

  // Fires when the web client sends recording settings
  // for a given recording
  onRecordingSettingsMessage(recordingName, recordingSettings) {
    // Save the recording settings to a json file
    this.debug('deprecated');
  }

  // Fires when the web client requests to
  // set the given channel's gain
  onSetChannelGainMessage(channelIndex, gain) {
    // Python considers the channels as one-based not zero-based so we increment
    // the index here
    channelIndex += 1;
    // Build the config command string that will be sent to the luchador box and
    // send it
    var commandString = 'schg ' + channelIndex.toString() + ',' + gain;
    this.queueCommand(commandString);
  }

  // Fires when the web client requests to
  // toggle the given channel's bias
  onTurnChannelBiasOnOrOffMessage(channelIndex, isOn, biasType) {
    // Python considers the channels as one-based not zero-based so we increment
    // the index here
    channelIndex += 1;
    // Python wants a 1 or 0 instead of true or false so we convert here
    var isOnString = '1';
    if (isOn == false) {
      isOnString = '0';
    }
    // Figure out which bias command to send (i.e. for bias p or bias n)
    var biasCommandString = 'schbp';
    if (biasType == 'n') {
      biasCommandString = 'schbn';
    }
    // Build the config command string that will be sent to the luchador box and
    // send it
    var commandString = biasCommandString + ' ' + channelIndex.toString() +
        ',' + isOnString;
    this.queueCommand(commandString);
  }

  onStartRecordingMessage(recordingSettings) {
    // Build the file path
    let recordingName = recordingSettings['site'] + '-' +
      recordingSettings['task'] + '-' +
      recordingSettings['participantId'] + '-' +
      recordingSettings['session'];
    // Replace spaces with underscores
    recordingName = recordingName.replace(' ', '_');
    var filePath = path.join(recordingSettings['directory'], recordingName) + '.csv';
    // Check if the file exists
    this.fs.exists(filePath, (exists) => {
      // If the file exists
      if (exists == true) {
        // Tell the web client about it
        this.top.getServerer().sendRecordingAlreadyExistsMessageToWebClient(filePath);
        // If the file does not exist
      } else {
        // Start recording
        this.startRecording(filePath, recordingSettings);
      }
    });
  }

  // Fires when the web client requests
  // impedance measurements to start.
  onStartImpedanceMessage() {
    var commandString = 'bi';
    this.queueCommand(commandString);
  }

  // Fires when the web client requests
  // impedance measurements to start.
  onStopImpedanceMessage() {
    var commandString = 'ei';
    this.queueCommand(commandString);
  }

  // Fires when the web client requests
  // internal test mode from amplifier.
  onSendTestCommandToAmplifer() {
    this.queueCommand('test');
  }

  handleVersionResponse(resp) {
    let match = resp.trim().match(/FW VERSION:([\d\.]+), HW VERSION:([\d\.]+)/);
    if (!match || (match.length != 3)) {
      this.debug('ERROR: malformed version string: "' + resp + '"');
    } else {
      this.fwVersion = match[1];
      this.hwVersion = match[2];
      this.top.getServerer().sendBioampVersionMessageToWebClient(this.fwVersion, this.hwVersion);
    }
  }

  ////////////////////////////////////////////////
  // utilities
  ////////////////////////////////////////////////

  sendCommand(command) {
    this.debug('send command: "' + command + '"');
    this.socket.send(command + '\r');
    this.pendingCommand = command;
  }

  sendNextCommand() {
    if (this.pendingCommand != null) {
      this.debug('ERROR: pendingCommand != null in sendNextCommand()');
      return;
    }
    let next = this.commandQueue.shift();
    if (next != null) {
      this.sendCommand(next);
    }
  }

  queueCommand(command) {
    if (!this.socket.getIsConnected()) {
      this.commandQueue.push(command);
    } else if (this.pendingCommand != null) {
      this.commandQueue.push(command);
    } else {
      this.sendCommand(command);
    }
  }

  writeHeader(stream, recordingSettings) {
    let preset = this.top.getPresetter().getPreset(recordingSettings['presetName']);
    if (preset == null) {
      this.debug('ERROR: preset "' + presetName + '" not found!');
      return false;
    }
    let header = {};

    let channelCount = preset.channelData.length;
    let channelData = preset.channelData;

    header['site'] = recordingSettings['site'];
    header['task'] = recordingSettings['task'];
    header['participantId'] = recordingSettings['participantId'];
    header['session'] = recordingSettings['session'];
    header['datetime'] = new Date().toISOString();
    header['channelCount'] = channelCount;
    header['timestampIndex'] = 0;
    header['markIndex'] = 1 + channelCount;
    header['unit'] = 'adc';
    header['sampleRate'] = 250;
    header['collectVersion'] = this.top.getVersion();
    if (this.hwVersion) {
      header['bioampId'] = 'luchador-' + this.hwVersion;
    }
    if (this.fwVersion) {
      header['bioampVersion'] = this.fwVersion;
    }

    header['channels'] = [];
    for (let i = 0; i < channelCount; i++) {
      header['channels'].push({
        'index': 1 + i,
        'gain': channelData[i].gain,
        'isOn': channelData[i].isOn,
        'label': channelData[i].label,
      });
    }

    let str = JSON.stringify(header) + '\r\n----\r\n';
    return stream.write(str);
  }

  // Start recording channel sensor data
  startRecording(recordingPath, recordingSettings) {
    // Create a stream to write to
    this.recordStream = this.fs.createWriteStream(recordingPath);
    // Output header.
    // TODO: need to handle the error here, but there's no way to return errors
    // to the front-end right now...
    this.writeHeader(this.recordStream, recordingSettings);
    // Turn on recording
    this.setAmRecording(true);
    // Tell the web client that recording has been turned on
    this.top.getServerer().sendRecordingStartedMessageToWebClient(recordingPath);
  }

  // Stop recording channel sensor data
  stopRecording() {
    this.debug('stopRecording');
    if (this.getAmRecording() == false) {
      return;
    }
    // Turn off recording
    this.setAmRecording(false);
    // Stop the stream writing
    this.recordStream.end();
    // Tell the web client that recording has been turned off
    this.top.getServerer().sendRecordingStoppedMessageToWebClient();
  }

  // Record the given channel data samples to the current record stream
  recordChannelDataToStream(channelDataString) {
    // Store the given data string so we can eliminate duplication on the next
    // pass
    this.previouslyRecordedDataString = channelDataString.toString();
    // Write the data string to the stream
    this.recordStream.write(channelDataString + '\r\n');
  }
}

