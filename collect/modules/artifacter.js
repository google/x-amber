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

artifacter.js

Detect artifacts in real time.
Instantiates an EpochLevelArtifacter -- which detects epoch level artifacts
Instantiates an AlwaysOnArtifacter -- which detects artifacts continuously

**********************************************************************/


'use strict'

const math = require('mathjs');
const jStat = require('jStat').jStat;
const ft = require('fourier-transform');
const hammingWindow = require('window-function/hamming');
const applyWindow = require('window-function/apply');
const fs = require('fs');
const path = require('path');

const EpochLevelArtifacter = require('./epoch-level-artifacter.js');
const AlwaysOnArtifacter = require('./always-on-artifacter.js');

module.exports = class Artifacter {
  constructor(top) {
    this.top = top;
    this.epochLevelArtifacter = new EpochLevelArtifacter(this);
    this.alwaysOnArtifacter = new AlwaysOnArtifacter(this);
    
    this.SAMPLING_FREQ = 250; // sample frequency (samples per second)
    this.NICKS_CONSTANT = 0.023517417908; // convert from adc to mV (also in dataer.js)
    this.NUM_CHANNELS = 3;
    this.debug = require('debug')('collect:artifacter');


    // this.ELECTRICAL_BAND_MOVING_AVERAGE_CONSTANT = 0.8;
    // this.electricalBandMovingAverage = null;

    this.getFeatureThresholds();
  }

  getTop() {
    return this.top;
  }

  receiveData(data) {
    let dataSplit = data.split(',');
    let channelData = dataSplit.slice(1, 1 + this.NUM_CHANNELS);
    
    // convert string array to number array and add to window
    let channelDataAsNumbers = channelData.map(Number);
    
    this.epochLevelArtifacter.addData(channelDataAsNumbers, dataSplit);
    this.alwaysOnArtifacter.addData(channelDataAsNumbers);    
  }

  getResultsFromArtifactModel(features) {  
    let isArtifact;
    for (let featureName in this.FEATURE_THRESHOLDS) {
      if (!(this.lte(features[featureName], this.FEATURE_THRESHOLDS[featureName]["max"])
      && this.gte(features[featureName], this.FEATURE_THRESHOLDS[featureName]["min"]))) {
        isArtifact = true;
        return isArtifact;
      }
    }
    isArtifact = false;
    return isArtifact;
  }


  getFeatures(data) {
    let windowSize = data.length;

    let signal = jStat(data).multiply(this.NICKS_CONSTANT);
    let fft = this.getFFT(signal, windowSize);
    
    let features = {
      "std": this.getStandardDeviation(signal),
      "mean": this.getMean(signal),
      "peakToPeak": this.getPeakToPeak(signal),
      "broadBand": this.getBroadBand(fft, windowSize),
      "electricalBand": this.getElectricalBand(fft, windowSize) // WILL DEPEND ON WINDOW_SIZE
    };
    return features
  }

  getStandardDeviation(signal) {
    return this.jStatToMathJS(signal.stdev());
  }

  getMean(signal) {
    return this.jStatToMathJS(signal.mean());
  }

  getPeakToPeak(signal) {
    return math.subtract(math.matrix(signal.max()), math.matrix(signal.min()));
  }

  getKurtosis(signal) {
    return this.jStatToMathJS(signal.kurtosis());
  }

  getBroadBand(signal, windowSize) {
    return this.getFFTBand(signal, 5, 59, windowSize);
  }

  getElectricalBand(fft, windowSize) {
    // Calculates the moving average of the electrical band due to typical oscillations in its power

    // If the frequency band is too narrow, there won't be any frequencies there due to the FFT frequencies being discrete
    // To determine the smallest frequency band, we need the sampling frequency and the window size

    let startBand = 60 - 1/2*this.SAMPLING_FREQ/windowSize;
    let endBand = 60 + 1/2*this.SAMPLING_FREQ/windowSize;
    let electricalBand = this.getFFTBand(fft, startBand, endBand, windowSize);
    return electricalBand
    
    // // Returns moving average of the electrical band
    // if (this.electricalBandMovingAverage == null) {
    //   this.electricalBandMovingAverage = electricalBand;
    // } else {
    //   this.electricalBandMovingAverage = math.add(math.multiply(this.ELECTRICAL_BAND_MOVING_AVERAGE_CONSTANT, this.electricalBandMovingAverage), 
    //     math.multiply(1-this.ELECTRICAL_BAND_MOVING_AVERAGE_CONSTANT, electricalBand));
    // }
    // return this.electricalBandMovingAverage
  }

  getFFT(signal, windowSize) {
    // Requires the signal length to be a power of 2.

    let signalArray = signal.transpose().toArray();
    let spectrums = [];

    // assumes n is even
    let frequencies = math.multiply(math.range(0, windowSize/2), this.SAMPLING_FREQ/windowSize);

    let windowedSignal, spectrum;
    for (let i=0; i<this.NUM_CHANNELS; i++) {
      windowedSignal = applyWindow(signalArray[i], hammingWindow);
      spectrum = ft(windowedSignal);
      spectrums.push(math.matrix(spectrum));
    }
    return {"frequencies": frequencies, "spectrum": spectrums};
  }

  getRelevantIndeces(minFreq, maxFreq, windowSize) {
    // Returns indeces in the frequency array bounded by minFreq and maxFreq
    return math.index(math.range(Math.ceil(minFreq*windowSize/this.SAMPLING_FREQ), Math.floor(maxFreq*windowSize/this.SAMPLING_FREQ), true));
  }

  getFFTBand(fft, minFreq, maxFreq, windowSize) {
    // get average power in the frequency bands bounded by minFreq and maxFreq
    let indeces = this.getRelevantIndeces(minFreq, maxFreq, windowSize);
    let relevantSpectrum = [];
    for (let i=0; i<this.NUM_CHANNELS; i++) {
      relevantSpectrum.push(math.mean(math.subset(fft.spectrum[i], indeces)));
    }
    return math.matrix(relevantSpectrum);
  }

  getFeatureThresholds() {
    fs.readFile(path.join(__dirname, '../artifact-detection-thresholds', 'thresholds.json'), (err, contents) => {
      this.FEATURE_THRESHOLDS = JSON.parse(contents);
    }); // runs asynchronously since the file contents are only required after ~1 second (this.WINDOW_SIZE samples)
  }

  // helper functions
  jStatToMathJS(o) {
    return math.matrix(o.slice());
  }

  lte(feature, max) {
    // checks if all channels are less than or equal to max
    if (max) {
      return !(math.sum(math.larger(feature, max)) > 0)
    } else {
      return true
    }
  }

  gte(feature, min) {
    // checks if all channels are greater than or equal to min
    if (min) {
      return !(math.sum(math.smaller(feature, min)) > 0)
    } else {
      return true
    }
  }
}

