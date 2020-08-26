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

serverer.js

This object serves up the web client that is the visualizer
via an http server.
It also acts as the communication conduit between the web client
and the node application.
Communication occurs via websocket between the two.
A corollary is that the websocket acts as a passthrough
so that the web client can send data to the node application
which can then send it to the boxerer object to send data
to the luchador box via tcp socket (and vice versa).

**********************************************************************/


'use strict';

// NOTE: extendWebSocket is duplicated in
// /public/webClient/modules/core/socketer.js

// extendWebSocket extends the normal WebSocket with on style handlers assuming
// that the incoming data is JSON with a 'type' and a 'data' key where type is
// the emit topic and the data is the root message. This makes the change from
// socket.io smoother.
function extendWebSocket(socket) {
  // Handle incoming messages.
  socket.onmessage = function(evt) {
    this.shimFnDict = this.shimFnDict || {};

    let packet = JSON.parse(evt.data);

    var fn = this.shimFnDict[packet['topic']];
    if (!fn) {
      console.log('Unhandled function: ', packet['topic']);
    }
    fn(packet['data']);
  }.bind(socket);

  // Acts like socket.io's version of .on()
  var shimOn =
      function(key, fn) {
    this.shimFnDict = this.shimFnDict || {};

    let fnDict = this.shimFnDict;
    fnDict[key] = fn;
  }

      // Use .register as the function to register handlers for various types.
      socket.register = shimOn.bind(socket);

  // Use .emitSend to send messages to other similarly extended sockets.
  socket.emitSend = function(topic, message) {
    this.send(JSON.stringify({
      'topic': topic,
      'data': message,
    }));
  }.bind(socket);
}


module.exports = class Serverer {
  constructor(top) {
    this.top = top;
    this.debug = require('debug')('collect:serverer');
    this.PUBLIC_FOLDER_PATH = './public';
    this.WEB_CLIENT_FOLDER_PATH = this.PUBLIC_FOLDER_PATH + '/webClient';
    this.WEB_CLIENT_INDEX_PATH = this.WEB_CLIENT_FOLDER_PATH + '/index.html';
    this.WEB_CLIENT_SOCKET_PORT = 7777;
    this.currentWebSocket = null;
    this.express = null;
    this.app = null;

    this.initHttpServer();
    this.initWebClientSocketServer();
  }

  // Create the http server that will serve up
  // the visualizer web client
  initHttpServer() {
    this.express = require('express');
    this.app = this.express();
    this.app.use(this.express.static(this.PUBLIC_FOLDER_PATH));

    this.app.get('/client', (req, res) => {
      res.sendFile(
          this.WEB_CLIENT_INDEX_PATH, {'root': this.PUBLIC_FOLDER_PATH});
    });

    var http = require('http');
    this.httpServer = http.createServer(this.app);
    this.httpServer.listen(this.WEB_CLIENT_SOCKET_PORT, () => {
      this.debug(
          'socket.io server listening on port', this.WEB_CLIENT_SOCKET_PORT);
    });
  }


  // Create the websocket server that will serve up
  // websocket connections to connected visualizer web clients
  initWebClientSocketServer() {
    var ws = require('ws');

    const io = new ws.Server({server: this.httpServer});

    // When the socket connection is established
    io.on('connection', (socket) => {
      this.debug('web socket connection opened', socket.id);

      // Set this socket as the current socket
      this.currentWebSocket = socket;

      // Add the emit style handlers.
      extendWebSocket(socket);

      // When the socket disconnects
      socket.on('close', function(closeCode, closeReason) {
        this.debug(
            'web socket connection closed:', closeCode + ',' + closeReason);
        this.currentWebSocket = null;
      }.bind(this));

      // When an error occurs on the underlying server.
      socket.on('error', (errorEvent) => {
        this.debug('web socket connection error', errorEvent);
      });

      // Listen for varieties of messages
      socket.register(
          'sendPresets', (message) => this.handleSendPresetsMessage(message));
      socket.register(
          'sendSerialPorts',
          (message) => this.handleSendSerialPortsMessage(message));
      socket.register(
          'startRecording',
          (message) => this.handleStartRecordingMessage(message));
      socket.register(
          'stopRecording',
          (message) => this.handleStopRecordingMessage(message));
      socket.register(
          'startImpedance',
          (message) => this.handleStartImpedanceMessage(message));
      socket.register(
          'stopImpedance',
          (message) => this.handleStopImpedanceMessage(message));
      socket.register(
          'savePreset', (message) => this.handleSavePresetMessage(message));
      socket.register(
          'connectToBox', (message) => this.handleConnectToBoxMessage(message));
      socket.register(
          'turnChannelOnOrOff',
          (message) => this.handleTurnChannelOnOrOffMessage(message));
      socket.register(
          'recordingSettings',
          (message) => this.handleRecordingSettingsMessage(message));
      socket.register(
          'sendVersion', (message) => this.handleSendVersionMessage(message));
      socket.register(
          'setChannelGain',
          (message) => this.handleSetChannelGainMessage(message));
      socket.register(
          'turnChannelBiasOnOrOff',
          (message) => this.handleTurnChannelBiasOnOrOffMessage(message));
      socket.register(
          'sendMark', (message) => this.handleSendMarkToAmplifier(message));
      socket.register(
          'sendTest', (message) => this.handleSendTestCommandToAmplifier(message))
    });
  }


  ////////////////////////////////////////////////
  // utilities
  ////////////////////////////////////////////////

  // Do something given that the web client
  // has requested the serial ports.
  handleSendSerialPortsMessage(message) {
    this.debug('handleSendSerialPortsMessage');
    var serialPorts = this.top.getBoxer().getSerialPortNames();
    var serialPortsString = JSON.stringify(serialPorts);
    var message = {
      'serialPorts': serialPortsString,
    };
    this.sendSerialPortsMessageToWebClient(message);
  }

  // Do something given that the web client
  // has requested the presets
  handleSendPresetsMessage(message) {
    this.debug('handleSendPresetsMessage');
    // Collect the presets and send them to the web client
    var presets = this.top.getPresetter().getPresets();
    var presetsString = JSON.stringify(presets);
    var message = {
      'presets': presetsString,
    };
    this.sendPresetsMessageToWebClient(message);
  }

  handleStartImpedanceMessage(message) {
    this.debug('handleStartImpedanceMessage');
    // Pass on the request to boxer
    this.top.getBoxer().onStartImpedanceMessage();
  }

  handleStopImpedanceMessage(message) {
    this.debug('handleStopImpedanceMessage');
    // Pass on the request to boxer
    this.top.getBoxer().onStopImpedanceMessage();
  }

  // Do something given that the web client
  // has requested that recording commence
  handleStartRecordingMessage(message) {
    this.debug('handleStartRecordingMessage', message);
    this.top.getBoxer().onStartRecordingMessage(message['recordingSettings']);
  }

  // Do something given that the web client
  // has requested that recording stop
  handleStopRecordingMessage(message) {
    // Instruct the boxer to stop recording
    this.top.getBoxer().stopRecording();
  }

  // Do something given that the web client
  // has requested the given preset be saved
  handleSavePresetMessage(message) {
    // Extract the preset data
    // and instruct the presetter to save it
    var preset = message['preset'];
    this.top.getPresetter().savePreset(preset);
  }

  // Do something given that the web client
  // has asked the node app to connect to
  // the luchador box at the given ip address
  handleConnectToBoxMessage(message) {
    // Extract the given ip and
    // instruct boxer to try to connect to that ip
    var port = message['port'];
    this.debug(message);
    this.debug('selected port', port);
    this.top.getBoxer().onConnectToBoxMessage(port);
  }

  // Do something given that the web client
  // has asked to turn a given channel on or off
  handleTurnChannelOnOrOffMessage(message) {
    // Extract the channel index, the on/off state, and
    // pass the info on to boxer
    var channelIndex = message['channelIndex'];
    var isOn = message['isOn'];
    this.top.getBoxer().onTurnChannelOnOrOffMessage(channelIndex, isOn);
  }

  // Do something given that the web client
  // has sent recording settings
  handleRecordingSettingsMessage(message) {
    // Extract the recording name and settings and
    // pass these on to boxer to handle it
    var recordingName = message['recordingName'];
    var recordingSettings = message['recordingSettings'];
    this.top.getBoxer().onRecordingSettingsMessage(
        recordingName, recordingSettings);
  }

  // Do something given that the web client
  // has requested the app's current version
  handleSendVersionMessage(message) {
    var version = this.top.getVersion();
    this.sendVersionMessageToWebClient(version);
  }

  // Do something given that the web client
  // has requested the app's current version
  handleSetChannelGainMessage(message) {
    // Extract the channel index, the gain level, and
    // pass the info onto boxer
    var channelIndex = message['channelIndex'];
    var gain = message['gain'];
    this.top.getBoxer().onSetChannelGainMessage(channelIndex, gain);
  }

  // Do something given that the web client
  // has requested to turn a given channel's bias
  // on or off
  handleTurnChannelBiasOnOrOffMessage(message) {
    // Extract the channel index, the on/off state, and
    // pass the info on to boxer
    var channelIndex = message['channelIndex'];
    var isOn = message['isOn'];
    var biasType = message['biasType'];
    this.top.getBoxer().onTurnChannelBiasOnOrOffMessage(
        channelIndex, isOn, biasType);
  }

  // Do something given that the web client
  // has requested to send a mark to the amplifier.
  handleSendMarkToAmplifier(message) {
    // Extract the mark.
    var mark = message['mark'];
    this.top.getBoxer().onSendMarkToAmplifier(mark);
  }

  handleSendTestCommandToAmplifier(message) {
    this.top.getBoxer().onSendTestCommandToAmplifer();
  }

  // Send the found serial ports to the web client
  sendSerialPortsMessageToWebClient(message) {
    this.sendMessageToWebClient('serialPorts', message);
  }

  // Send the given presets to the web client
  sendPresetsMessageToWebClient(message) {
    this.sendMessageToWebClient('presets', message);
  }

  // Send a string of data (with a data sample for each channel)
  // to the web client.
  // This data was received via boxer tcp/ip socket
  // and boxer passed it here
  sendChannelDataStringMessageToWebClient(channelDataString) {
    var message = {'channelDataString': channelDataString};
    this.sendMessageToWebClient('channelDataString', message);
  }

  // Send a string of impedance data (with data for a single channel)
  // to the web client.
  sendImpedanceDataStringToWebClient(impedanceDataString) {
    var message = {'impedanceDataString': impedanceDataString};
    this.sendMessageToWebClient('impedanceDataString', message);
  }

  // Send a message to the web client that recording has started
  sendRecordingStartedMessageToWebClient(recordingName) {
    this.debug('sendRecordingStartedMessageToWebClient', recordingName);
    var message = {'recordingName': recordingName};
    this.sendMessageToWebClient('recordingStarted', message);
  }

  // Send a message to the web client that recording has stopped
  sendRecordingStoppedMessageToWebClient() {
    var message = {};
    this.sendMessageToWebClient('recordingStopped', message);
  }

  // Send a message to the web client that a preset was saved successfully
  sendPresetSavedMessageToWebClient() {
    var message = {};
    this.sendMessageToWebClient('presetSaved', message);
  }

  // Send a message to the web client that the app
  // has been unable to connect to the box
  sendUnableToConnectToChannelDataSocketMessageToWebClient() {
    var message = {};
    this.sendMessageToWebClient('unableToConnectToChannelDataSocket', message);
  }

  // Send a message to the web client that the node app is connected
  // to the luchador channel data tcp/ip socket
  sendChannelDataSocketConnectedMessageToWebClient() {
    var message = {};
    this.sendMessageToWebClient('channelDataSocketConnected', message);
  }

  // Send a message to the web client that the channel data socket closed
  sendChannelDataSocketClosedMessageToWebClient() {
    var message = {};
    this.sendMessageToWebClient('channelDataSocketClosed', message);
  }

  // Send a message to the web client that the channel data socket encountered
  // an error
  sendChannelDataSocketErrorMessageToWebClient() {
    var message = {};
    this.sendMessageToWebClient('channelDataSocketError', message);
  }

  // Send a message to the web client containing the always on artifact update
  sendAlwaysOnArtifactUpdateToWebClient(isArtifact) {
    var message = {'isArtifact': isArtifact};
    this.sendMessageToWebClient('alwaysOnArtifactUpdate', message);
  }

  // Send a message to the web client containing the epoch level artifact update
  sendEpochLevelArtifactUpdateToWebClient(isArtifact) {
    var message = {'isArtifact': isArtifact};
    this.sendMessageToWebClient('epochLevelArtifactUpdate', message);
  }

  // Send a message to the web client containing the current app version
  sendVersionMessageToWebClient(version) {
    var message = {'version': version};
    this.sendMessageToWebClient('version', message);
  }

  // Send a message to the web client that a recording
  // with the given name already exists
  sendRecordingAlreadyExistsMessageToWebClient(recordingName) {
    var message = {'recordingName': recordingName};
    this.sendMessageToWebClient('recordingAlreadyExists', message);
  }

  sendRecordingErrorMessageToWebClient(error) {
    this.sendMessageToWebClient('recordingError', error);
  }

  sendBioampVersionMessageToWebClient(fwVersion, hwVersion) {
    this.sendMessageToWebClient('bioampVersion', {
      'fwVersion': fwVersion,
      'hwVersion': hwVersion,
    })
  }

  // Send the given message with the given message type to the web client
  sendMessageToWebClient(messageType, message) {
    if (this.currentWebSocket == null) {
      // this.debug('error: sendMessageToWebClient socket is null');
      return;
    }
    this.currentWebSocket.emitSend(messageType, message);
  };
}