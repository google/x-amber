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
 package.js

 Builds distributable binaries.
 */

const packager = require('electron-packager');
const archiver = require('archiver');
const fs = require('fs-extra');
const path = require('path');
const pjson = require('../package.json');
const version = require('../lib/version.js');

const BUILDDIR = 'build';
const BINDIR = pjson.name + '-' + version + '-' +  process.platform;
const BINNAME = pjson.name + '-' + process.platform;
const BINPATH = path.join(BUILDDIR, BINDIR, BINNAME);
const ARCHIVENAME = BINDIR + '.zip';

const NATIVE_MODULES = [
    'node_modules/opn/xdg-open',
    'node_modules/serialport/build/Release/serialport.node',
];

const COPY_DIRS = [
    'presets',
    'artifact-model'
];

(async function() {

  // package app
  let rc = await packager({
    dir: '.',
    icon: 'app/images/collect_logo_v2.ico',
    out: BUILDDIR,
  });

  let outpath = rc[0];

  // copy front-end and other folders
  for (let p of COPY_DIRS) {
    fs.copySync(p, path.join(outpath, p));
  }

  // include version in folder name
  fs.renameSync(outpath, path.join(BUILDDIR, BINDIR));

  // zip up the whole folder
  let os = fs.createWriteStream(path.join(BUILDDIR, ARCHIVENAME));
  let ar = archiver('zip', {
    zlib: { level: 9 }
  });

  os.on('close', function() {
    console.log('Successfully built', ARCHIVENAME);
  });

  ar.on('warning', function(_err) {
    throw _err;
  });

  ar.on('error', function(_err) {
    throw _err;
  });

  ar.pipe(os);
  let OUTDIR = path.join(BUILDDIR, BINDIR);
  ar.directory(OUTDIR, BINDIR);
  ar.finalize();

})();
