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

/**
 app.js

 Runs the server and opens a browser.
 */

'use strict';

process.env.DEBUG = 'collect:*';

const opn = require('opn');
const { app, BrowserWindow } = require('electron');

// Run server
const Presetter = require('./modules/presetter.js');
const Boxer = require('./modules/boxer.js');
const Serverer = require('./modules/serverer.js');
const Artifacter = require('./modules/artifacter.js');

// The top level container object
// for the app singletons
class Top {
  constructor() {
    this.version = require("./lib/version.js");

    // command-line arguments
    let file = '';
    let n = app.isPackaged ? 2 : 3;
    if (process.argv.length == n) {
      process.argv.forEach((val, index) => {
        // optional argument: use file as data source
        if (index == n - 1) {
          file = val;
        }
      });
    }

    this.presetter = new Presetter(this);
    this.boxer = new Boxer(this, file);
    this.serverer = new Serverer(this);
    this.artifacter = new Artifacter(this);
    this.win = null; // created later
  }

  getVersion() {
    return this.version;
  }
  getPresetter() {
    return this.presetter;
  }
  getBoxer() {
    return this.boxer;
  }
  getServerer() {
    return this.serverer;
  }
  getArtifacter() {
    return this.artifacter;
  }
  getWindow() {
    return this.win;
  }
}

const server = new Top();

function createWindow() {
  server.win = new BrowserWindow({minWidth: 1536, minHight: 864});
  server.win.maximize();
  server.win.loadFile('app/index.html');
  if (process.env.DEV == '1') {
    server.win.webContents.openDevTools();
  }
}

app.on('ready', createWindow);
app.on('window-all-closed', () => {
  app.quit();
});

/*
// Open webclient in chrome
const WEBCLIENT = 'http://localhost:7777/webClient';

let CHROME;
switch (process.platform) {
  case 'darwin':
    CHROME = 'google chrome';
    break;
  case 'linux':
    CHROME = 'google-chrome';
    break;
  case 'win32':
    CHROME = 'chrome';
    break;
  default:
    console.log('Unsupported platform', process.platform);
}

if (process.env.DEV != '1') {
  opn(WEBCLIENT, { app: CHROME });
}
*/
