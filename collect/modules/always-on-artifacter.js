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

always-on-artifacter.js

Constantly checks for artifacts and updates web client.

**********************************************************************/

'use strict'

module.exports = class AlwaysOnArtifacter {
  constructor(artifacter) {
    this.top = artifacter;

    this.STRIDE = 10; // controls the frequency of checking for artifacts (# of samples)
    this.WINDOW_SIZE = 256; // # of samples (~4 sec)
    
    this.data = [];
  }

  getArtifacter() {
    return this.top;
  }

  addData(channelDataAsNumbers) {
    this.data.push(channelDataAsNumbers);
    
    // window full
    if (this.data.length == this.WINDOW_SIZE) {
      this.checkForArtifact();
      
      // remove the last stride of data
      this.data = this.data.slice(this.STRIDE);
    }
  }

  checkForArtifact() {
    if (this.data.length != this.WINDOW_SIZE) {
      this.getArtifacter().debug('ERROR: Incorrect window size');
      return
    } else {
      let features = this.getArtifacter().getFeatures(this.data);
      let isArtifact = this.getArtifacter().getResultsFromArtifactModel(features);
      this.getArtifacter().top.getServerer().sendAlwaysOnArtifactUpdateToWebClient(isArtifact);
    }
  }
}