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

epoch-level-artifacter.js

Analyzes individual epochs and determines if they contain artifacts.

**********************************************************************/

'use strict';

module.exports = class EpochLevelArtifacter {
  constructor(artifacter) {
    this.top = artifacter;

    this.MARKER_INDEX = 32;

    // task specific constants, TODO: CHANGE THEM
    this.START_OF_EPOCH_MARKER_VALUES = [3,777];
    // this.EPOCH_DURATION must be a power of 2 for the FF
    // this.EPOCH_DURATION must be greater than this.PRE_MARKER_DURATION
    this.EPOCH_DURATION = 256; // (256 = ~977 ms)
    this.PRE_MARKER_DURATION = 100; // (100 = 400 ms)

    this.preStimulusData = [];
    this.epochBegun = false;
    this.epochData = [];

  }

  getArtifacter() {
    return this.top;
  }

  addData(channelDataAsNumbers, dataSplit) {
    if (this.epochBegun) {
      this.epochData.push(channelDataAsNumbers);

      if (this.isEpochOver()) {
        this.epochBegun = false;
        this.checkForArtifact();
        this.epochData = [];
      }
    }
    else {
      this.preStimulusData.push(channelDataAsNumbers);
      this.preStimulusData.splice(this.PRE_MARKER_DURATION);
      
      if (this.hasEpochBegun(dataSplit)) {
        this.startEpoch();
      }
    }
  }

  checkForArtifact() {
    if (this.epochData.length != this.EPOCH_DURATION) {
      this.getArtifacter().debug('ERROR: Incorrect window size' + this.epochData.length);
      return
    } else {
      let features = this.getArtifacter().getFeatures(this.epochData);
      let isArtifact = this.getArtifacter().getResultsFromArtifactModel(features);

      // TODO: Can display it in the front end if we want to
      this.getArtifacter().top.getServerer().sendEpochLevelArtifactUpdateToWebClient(isArtifact);

      // TODO: Remove this when Stimulus communication is working
      if (isArtifact) {
        console.log('Bad trial');
      } else {
        console.log('Good trial :)');
      }

    }
  }


  // helper functions

  hasEpochBegun(dataSplit) {
    let markerValue = parseInt(dataSplit[this.MARKER_INDEX + 1]); // + 1 to ignore time
    return this.START_OF_EPOCH_MARKER_VALUES.includes(markerValue)
  }

  startEpoch() {
    if (this.epochBegun) {
      this.getArtifacter().debug('Epoch already started');
    } else if (this.preStimulusData.length != this.PRE_MARKER_DURATION) {
      this.getArtifacter().debug('Not enough pre stimulus data collected');
    } else {
      this.epochBegun = true;
      this.epochData.push(...this.preStimulusData);
    }
  }

  isEpochOver() {
    return this.epochData.length == this.EPOCH_DURATION;
  }

}