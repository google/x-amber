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

socketer.js

This is the object that handles all web socket
two-way communication with the server.
As messages are received they are routed appropriately
and messages are also sent to the server.
Note:
Some outgoing messages are intended to end at the server (e.g. "start
recording"), while other messages use the server as a pass through, where the
server receives the given message and routes it to the box.

**********************************************************************/


'use strict';

// NOTE: extendWebSocket is duplicated in /modules/serverer.js

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

class Socketer {
  constructor(top) {
    this.top = top;
    this.createSocket();
  }

  // Create the web socket that automagically
  // connects to the server upon creation
  // and listen for connection-related events
  // and specify the messages to listen for
  // along with their respective callbacks
  createSocket() {
    // Create the socket
    this.socket = new WebSocket('ws://localhost:7777');

    extendWebSocket(this.socket);

    // Listen for connect events and
    // print them to the terminal
    this.socket.onopen = function() {
      this.top.onConnectionWithServer();
    }.bind(this);

    // Listen for a variety of messages
    // and define the callbacks for each
    this.socket.register(
        'presets', (message) => this.handlePresetsMessage(message));
    this.socket.register(
        'serialPorts', (message) => this.handleSerialPortsMessage(message));
    this.socket.register(
        'channelDataString',
        (message) => this.handleChannelDataStringMessage(message));
    this.socket.register(
        'impedanceDataString',
        (message) => this.handleImpedanceDataStringMessage(message));
    this.socket.register(
        'recordingStarted',
        (message) => this.handleRecordingStartedMessage(message));
    this.socket.register(
        'recordingStopped',
        (message) => this.handleRecordingStoppedMessage(message));
    this.socket.register(
        'presetSaved', (message) => this.handlePresetSavedMessage(message));
    this.socket.register(
        'unableToConnectToChannelDataSocket',
        (message) =>
            this.handleUnableToConnectToChannelDataSocketMessage(message));
    this.socket.register(
        'channelDataSocketConnected',
        (message) => this.handleChannelDataSocketConnectedMessage(message));
    this.socket.register(
        'channelDataSocketClosed',
        (message) => this.handleChannelDataSocketClosedMessage(message));
    this.socket.register(
        'channelDataSocketError',
        (message) => this.handleChannelDataSocketErrorMessage(message));
    this.socket.register(
        'version', (message) => this.handleVersionMessage(message));
    this.socket.register(
        'recordingAlreadyExists',
        (message) => this.handleRecordingAlreadyExistsMessage(message));
    this.socket.register(
        'recordingError',
        (message) => this.handleRecordingErrorMessage(message));
    this.socket.register(
        'bioampVersion',
        (message) => this.handleBioampVersionMessage(message));
    this.socket.register(
        'alwaysOnArtifactUpdate',
        (message) => this.handleAlwaysOnArtifactUpdateMessage(message));
  }

  // Handle a message from the server
  // containing all the stored presets
  handlePresetsMessage(message) {
    // Extract the presets data
    var presets = JSON.parse(message['presets']);
    this.top.getPresetter().onPresetsMessageReceived(presets);
  }

  // Handle a message from the server
  // containing all the stored presets
  handleSerialPortsMessage(message) {
    // Extract the presets data
    var serialPorts = JSON.parse(message['serialPorts']);
    // Pass them ?
    this.top.getPorter().onSerialPortsMessageReceived(serialPorts);
  }

  // Handle a message from the server
  // containing a channel data string
  handleChannelDataStringMessage(message) {
    // Extract the channel data string
    var channelDataString = message['channelDataString'];
    // Pass the data on to dataer
    this.top.getDataer().onChannelDataStringMessageReceived(channelDataString);
  };

  // Handle a message from the server
  // containing impedance data
  handleImpedanceDataStringMessage(message) {
    // Extract the impedance data string
    var impedanceDataString = message['impedanceDataString'];
    // Pass on the impedance data to dataer
    this.top.getDataer().onImpedanceDataStringMessageReceived(
        impedanceDataString);
  }

  // Handle a message from the server
  // indicating that recording has started
  // along with the name of that recording
  handleRecordingStartedMessage(message) {
    // Extract the recording name
    var recordingName = message['recordingName'];
    // Pass on the recording name to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onRecordingStartedMessageReceived(recordingName);
  }

  // Handle a message from the server
  // indicating that recording has stopped
  handleRecordingStoppedMessage(message) {
    // Pass on this information to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onRecordingStoppedMessageReceived();
  }

  // Handle a message from the server
  // indicating that the current preset was saved
  handlePresetSavedMessage(message) {
    // Pass on this information to the upper controls
    this.top.getDisplayer().getUpperControls().onPresetSavedMessageReceived();
  }

  // Handle a message from the server
  // that the server is unable to connect to the box
  handleUnableToConnectToChannelDataSocketMessage(message) {
    // Pass on this information to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onUnableToConnectToChannelDataSocketMessageReceived();
  }

  // Handle a message from the server
  // that the server connected to the box's
  // data socket
  handleChannelDataSocketConnectedMessage(message) {
    // Pass on this information to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onChannelDataSocketConnectedMessageReceived();
  }

  // Handle a message from the server
  // that the server data socket disconnected from the box
  handleChannelDataSocketClosedMessage(message) {
    // Pass on this information to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onChannelDataSocketClosedMessageReceived();
  }

  // Handle a message from the server
  // that a data socket error occurred between
  // the server and the box
  handleChannelDataSocketErrorMessage(message) {
    // Pass on this information to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onChannelDataSocketErrorMessageReceived();
  }

  handleAlwaysOnArtifactUpdateMessage(message) {
    let artifact = message;
    this.top.getDisplayer().updateArtifact(artifact);
  }

  // Handle a message from the server
  // that contains the current app version
  handleVersionMessage(message) {
    var version = message['version'];
    // Pass on this information to the lower controls
    this.top.getDisplayer().getLowerControls().onVersionMessageReceived(
        version);
  }

  // Handle a message from the server
  handleRecordingAlreadyExistsMessage(message) {
    var recordingName = message['recordingName'];
    // Pass on this information to the upper controls
    this.top.getDisplayer()
        .getUpperControls()
        .onRecordingAlreadyExistsMessageReceived(recordingName);
  }

  handleRecordingErrorMessage(message) {
    this.top.onRecordingErrorMessage(message);
  }

  handleBioampVersionMessage(message) {
    this.top.onBioampVersionMessage(message);
  }

  // Send a message to the server
  // asking it to send back all known serial ports.
  sendSendSerialPortsMessageToServer() {
    var message = {};
    this.sendMessageToServer('sendSerialPorts', message);
  }

  // Send a message to the server
  // asking it to send back all known presets
  sendSendPresetsMessageToServer() {
    var message = {};
    this.sendMessageToServer('sendPresets', message);
  }

  // Send a message to the server
  // asking it to start recording data
  // and save it a file with the given recordingSettings in a user specified
  // directory
  sendStartRecordingMessageToServer(recordingSettings) {
    var message = {'recordingSettings': recordingSettings};
    this.sendMessageToServer('startRecording', message);
  }

  // Send a message to the server
  // asking it to stop recording
  sendStopRecordingMessageToServer() {
    var message = {};
    this.sendMessageToServer('stopRecording', message);
  }

  // Send a message to the server
  // asking it to start impedance measurements.
  sendStartImpedanceToServer() {
    var message = {};
    this.sendMessageToServer('startImpedance', message);
    this.top.getDataer().impedanceEnabled = true;
  }

  // Send a message to the server
  // asking it to stop impedance measurements.
  sendStopImpedanceToServer() {
    var message = {};
    this.sendMessageToServer('stopImpedance', message);
    this.top.getDataer().impedanceEnabled = false;
  }

  // Send a message to the server
  // asking it to save the given preset
  sendSavePresetMessageToServer(preset) {
    var message = {'preset': preset};
    this.sendMessageToServer('savePreset', message);
  }

  // Send a message to the server
  // asking it to connect to the box
  // using the given ip
  sendConnectToBoxMessageToServer(port) {
    var message = {'port': port};
    this.sendMessageToServer('connectToBox', message);
  }

  // Send a message to the server
  // asking it to turn the given channel
  // on or off
  sendTurnChannelOnOrOffMessageToServer(channelIndex, isOn) {
    var message = {'channelIndex': channelIndex, 'isOn': isOn};
    this.sendMessageToServer('turnChannelOnOrOff', message);
  }

  // Send a message to the server
  // asking it save the given recording settings
  // to a file with the given recording name
  sendRecordingSettingsMessageToServer(recordingName, recordingSettings) {
    var message = {
      'recordingName': recordingName,
      'recordingSettings': JSON.stringify(recordingSettings)
    };
    this.sendMessageToServer('recordingSettings', message);
  }

  // Send a message to the server
  // asking it to send back the current app version
  sendSendVersionMessageToServer() {
    var message = {};
    this.sendMessageToServer('sendVersion', message);
  }

  // Send a message to the server
  // asking it to turn the given channel's bias
  // of the given bias type (i.e. 'n' or 'p')
  // on or off
  sendTurnChannelBiasOnOrOffMessageToServer(channelIndex, isOn, biasType) {
    var message = {
      'channelIndex': channelIndex,
      'isOn': isOn,
      'biasType': biasType
    };
    this.sendMessageToServer('turnChannelBiasOnOrOff', message);
  }

  // Send a message to the server
  // asking it to set the given channel's gain
  // to the given gain level
  sendSetChannelGainMessageToServer(channelIndex, gain) {
    var message = {'channelIndex': channelIndex, 'gain': gain};
    this.sendMessageToServer('setChannelGain', message);
  }

  // Send a message to the server
  // asking it to send a given mark
  // to the amplifier.
  sendMarkToServer(mark) {
    var message = {
      'mark': mark,
    };
    this.sendMessageToServer('sendMark', message);
  }

  sendTestCommandMessageToServer() {
    this.sendMessageToServer('sendTest', {});
  }

  // Send a message to the server
  // of the given type and containing
  // the given message content
  sendMessageToServer(messageType, message) {
    if (this.socket.connected == false) {
      console.log(
          'socket is not connected, cannot send message', messageType, message);
      return;
    }

    var json = JSON.stringify(message);
    console.log(
        'sending message to server of type: ' + messageType +
        ' and content: ' + json);
    this.socket.emitSend(messageType, message);
  }
}
