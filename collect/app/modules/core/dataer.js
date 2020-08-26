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

dataer.js

This object receives and stores all channel data
including sensor readings and impedances.
FFT analysis is performed here as well.
Baselining, where data values are positioned relative
to a running average of a given number of samples is performed here.
This object defines important constants that significantly affect
how the data is visualized.
One constant of particular importance is ASSUMED_TIME_BETWEEN_CHANNELS.
This constant specifies the assumed time delta between data samples
(we need this because the data samples have no timestamp, only an ever
incrementing index that can be used to infer the timestamp, assuming
a given time delta between the samples.)
Since the current data sampling rate is 250Hz
the assume time delta is 4ms.

**********************************************************************/


'use strict'

class Dataer {
  constructor(top) {
    this.top = top;
    this.NUM_DATA_CHANNELS = this.top.getConfig().NUM_DATA_CHANNELS;
    this.MAX_NUM_SAMPLES_PER_CHANNEL = 1250;  // 2500;
    this.ASSUMED_TIME_BETWEEN_SAMPLES = 4;    // milliseconds
    this.ASSUMED_SAMPLE_RATE = 1000 / this.ASSUMED_TIME_BETWEEN_SAMPLES;  // Hz
    this.FFT_SAMPLE_SIZE = 256;  // 512;
    this.MIN_FREQUENCY_AMPLITUDE = 0;
    this.MAX_FREQUENCY_AMPLITUDE = 100;
    this.BASELINE_SAMPLE_SIZE = 500;
    this.V_REF = 4.5;
    //.conversion factor from bioamp data to uv
    this.CHANNEL_DATA_SCALE_FACTOR = (2 * this.V_REF / 24) / (2 << 23) * 1000000;
    this.MARKER_INDEX = 32;
    this.NO_MARKER_VALUE = 0;
    this.previousTimestampIndex = null;
    this.RAILING_MIN_SAMPLE_SIZE = 500;
    this.baselineMinDataValue = -500;  // microvolts
    this.baselineMaxDataValue = 500;   // microvolts
    this.impedanceEnabled = false;
    this.useFilter = false;
    this.lowPassIirFilter = [];
    this.highPassIirFilter = [];
    this.previousFiltered = [];

    this.initChannelData();
    this.initImpedanceData();
    this.initMarkerData();
  }

  initChannelData() {
    this.channelData = [];
    for (var i = 0; i < this.NUM_DATA_CHANNELS; i++) {
      this.channelData[i] = [];
    }
  }

  initImpedanceData() {
    this.impedanceData = [];
    for (var i = 0; i < this.NUM_DATA_CHANNELS; i++) {
      this.impedanceData[i] = null;
    }
  }

  initMarkerData() {
    this.markerData = [];
  }


  ////////////////////////////////////////////////
  // accessors
  ////////////////////////////////////////////////

  getChannelData(channelIndex) {
    return this.channelData[channelIndex];
  }
  getImpedanceData() {
    return this.impedanceData;
  }
  getMarkerData() {
    return this.markerData;
  }
  getTimeDomainData(channelIndex) {
    if (!this.useFilter) {
      return this.channelData[channelIndex];
    } else {
      return this.calculateFilteredTimeDomainData(channelIndex);
    }
  }
  getFrequencyDomainData(channelIndex) {
    return this.calculateFftOfTimeDomainData(channelIndex);
  }
  getMaxNumSamplesPerChannel() {
    return this.MAX_NUM_SAMPLES_PER_CHANNEL;
  }
  getAssumedTimeBetweenSamples() {
    return this.ASSUMED_TIME_BETWEEN_SAMPLES;
  }
  getAssumedSampleRate() {
    return this.ASSUMED_SAMPLE_RATE;
  }
  getMinFrequencyAmplitude() {
    return this.MIN_FREQUENCY_AMPLITUDE;
  }
  getMaxFrequencyAmplitude() {
    return this.MAX_FREQUENCY_AMPLITUDE;
  }
  getBaselineMinDataValue() {
    return this.baselineMinDataValue;
  }
  setBaselineMinDataValue(newValue) {
    this.baselineMinDataValue = newValue;
  }
  getBaselineMaxDataValue() {
    return this.baselineMaxDataValue;
  }
  setBaselineMaxDataValue(newValue) {
    this.baselineMaxDataValue = newValue;
  }


  ////////////////////////////////////////////////
  // callbacks
  ////////////////////////////////////////////////

  // Fires when a channel data string has been received from the node app.
  // This string contains sensor readings from each channel that is on,
  // or zero if that channel is not on.
  // The string is a comma separated list, with the first entry
  // being the time stamp index, an integer that increments by one
  // for each new data sample, and then the next 30 entries are
  // the channel sensor data values.
  // This is the time domain data.
  onChannelDataStringMessageReceived(channelDataString) {
    // Get all the separate entries in the string.
    // Trim before since sometimes padded strings are used for
    // latency optimization.
    var dataSplit = channelDataString.trim().split(',');
    // Extract the timeStamp index
    var timeStampIndex = parseInt(dataSplit[0]);
    // Calculate the timeStamp
    var timeStamp = timeStampIndex * this.ASSUMED_TIME_BETWEEN_SAMPLES;
    // Remove the timestamp from the split array
    dataSplit.shift();
    // Loop through all the channel entries
    for (var channelIndex = 0; channelIndex < dataSplit.length;
         channelIndex++) {
      // If the given entry index is less than the number of data channels
      if (channelIndex < this.NUM_DATA_CHANNELS) {
        // Create the data sample dictionary
        // Note: parseInt drastically improves performance!!
        var sample = {
          'value': parseInt(
              dataSplit[channelIndex] * this.CHANNEL_DATA_SCALE_FACTOR),
          'timeStamp': timeStamp
        };
        // Store the sample data
        this.pushSampleOntoChannel(channelIndex, sample);
        // If the given entry index is the index for markers
      } else if (channelIndex == this.MARKER_INDEX) {
        // Extract the marker value
        var value = parseInt(dataSplit[channelIndex]);
        // If the marker value is not the value "no marker"
        // indicating that a marker was sent
        if (value != this.NO_MARKER_VALUE) {
          var markerDatum = {'value': value, 'timeStamp': timeStamp};
          this.pushMarkerDatumOntoMarkerData(markerDatum);
        }
      }
    }
  }

  // Fires when an impedance data string message
  // has been received from the node app.
  // The impedance data string is of the format
  // CH CHANNEL_NUMBER=IMPEDANCE_VALUE\n\r
  onImpedanceDataStringMessageReceived(impedanceDataString) {
    // Get the channel and impedance value string
    var dataSplit = impedanceDataString.split(' ');
    // Split that string into channel and impedance
    var impedanceSplit = dataSplit[1].split('=');
    // Extract the channel index and zero index it.
    var channelIndex = parseInt(impedanceSplit[0]) - 1;
    // Extrace the impedance
    var impedance = parseInt(impedanceSplit[1]);
    // Store the value
    this.getImpedanceData()[channelIndex] = impedance;
  }

  ////////////////////////////////////////////////
  // filters
  ////////////////////////////////////////////////

  createLowPassFilter() {
    //  Instance of a filter coefficient calculator
    var lowPassIirCalculator = new Fili.CalcCascades();

    // calculate filter coefficients
    var lowPassIirFilterCoeffs = lowPassIirCalculator.lowpass({
      order: this.lowPassOrder,  // cascade 3 biquad filters (max: 12)
      characteristic: 'butterworth',
      Fs: 250,           // sampling frequency
      Fc: this.lowPass,  // cutoff frequency / center frequency for bandpass,
                         // bandstop, peak
      preGain:
          false  // adds one constant multiplication for highpass and lowpass
    });

    // create a filter instance from the calculated coeffs
    return new Fili.IirFilter(lowPassIirFilterCoeffs);
  }

  createHighPassFilter() {
    //  Instance of a filter coefficient calculator
    var highPassIirCalculator = new Fili.CalcCascades();

    // calculate filter coefficients
    var highPassIirFilterCoeffs = highPassIirCalculator.highpass({
      order: this.highPassOrder,  // cascade 3 biquad filters (max: 12)
      characteristic: 'butterworth',
      Fs: 250,            // sampling frequency
      Fc: this.highPass,  // cutoff frequency / center frequency for bandpass,
                          // bandstop, peak
      preGain:
          false  // adds one constant multiplication for highpass and highpass
    });

    // create a filter instance from the calculated coeffs
    return new Fili.IirFilter(highPassIirFilterCoeffs);
  }

  createFilter(channelIndex) {
    if (!this.lowPass || !this.lowPassOrder || !this.highPass || !this.highPassOrder) {
      return;
    }

    this.lowPassIirFilter[channelIndex] = this.createLowPassFilter();

    this.highPassIirFilter[channelIndex] = this.createHighPassFilter();
  }

  toggleFilter(on) {
    this.useFilter = on;
    // discard previous state when disabling filter
    if (!on) {
      this.lowPassIirFilter = [];
      this.highPassIirFilter = [];
      this.previousFiltered = [];
    }
  }

  ////////////////////////////////////////////////
  // utilities
  ////////////////////////////////////////////////

  // Store the given data sample onto the given channel's history
  pushSampleOntoChannel(channelIndex, sample) {
    this.channelData[channelIndex].push(sample);
    // If the maximum history size has been exceeded, push off the oldest sample
    if (this.channelData[channelIndex].length >
        this.MAX_NUM_SAMPLES_PER_CHANNEL) {
      this.channelData[channelIndex].shift();
    }
  }

  // Store the given marker datum in the marker data
  pushMarkerDatumOntoMarkerData(markerDatum) {
    this.markerData.push(markerDatum);
  }

  // Remove the given marker datum from the marker data
  removeMarkerDatumFromMarkerData(markerDatum) {
    var markerDatumIndex = this.getMarkerData().indexOf(markerDatum);
    this.getMarkerData().splice(markerDatumIndex, 1);
  }

  calculateFilteredTimeDomainData(channelIndex) {
    var x = this.channelData[channelIndex];
    var old_slice = null;
    var new_slice = null;
    if (!this.previousFiltered[channelIndex] ||
        this.previousFiltered[channelIndex].length != x.length ||
        this.previousFiltered[channelIndex]
                             [this.previousFiltered[channelIndex].length - 1]
                             ['timeStamp'] < x[0]['timeStamp']) {
      // No reasonable previous filtered data so create a new filter.
      this.createFilter(channelIndex);
      old_slice = [];
      new_slice = x;
    } else {
      let i = this.previousFiltered[channelIndex].length - 1;
      while (i >= 0) {
        if (this.previousFiltered[channelIndex][i]['timeStamp'] <=
            x[0]['timeStamp']) {
          break;
        }
        i -= 1;
      }
      old_slice = this.previousFiltered[channelIndex].slice(i);
      new_slice = x.slice(old_slice.length);
    }
    var filtered = new_slice.map(i => i['value']);
    var afterHighPass = this.highPassEnabled ? this.highPassIirFilter[channelIndex].multiStep(
        filtered, false) : filtered;  // true means side effect overwrite.
    var afterBothPass = this.lowPassEnabled? this.lowPassIirFilter[channelIndex].multiStep(
        afterHighPass, false) : afterHighPass;  // true means side effect overwrite.
    let out = old_slice;
    for (let i = 0; i < afterBothPass.length; i++) {
      out.push({
        'value': afterBothPass[i],
        'timeStamp': new_slice[i]['timeStamp'],
      });
    }
    this.previousFiltered[channelIndex] = out;
    return out;
  }

  // Calculate the FFT for the given channel.
  // We calculate over a subset of the current history
  // for performance reasons.
  // This subset starts at NOW, and goes back in time in the history
  // by a fixed amount of samples.
  // note:
  // Our range of frquencies returned here are in [0Hz,
  // (ASSUMED_SAMPLE_RATE/2)Hz] Also, the number of bins returned here will be
  // (FFT_SAMPLE_SIZE / 2)
  calculateFftOfTimeDomainData(channelIndex) {
    // Get the time domain data for this channel
    var timeDomainData = this.getTimeDomainData(channelIndex);
    // If we don't have enough samples, then exit
    if (timeDomainData.length < this.FFT_SAMPLE_SIZE) {
      return null;
    }

    // Specify the time domain indices start and stop
    var startIndex = timeDomainData.length - 1 - this.FFT_SAMPLE_SIZE;
    var stopIndex = timeDomainData.length - 1;
    var timeDomainData = timeDomainData.slice(startIndex, stopIndex);
    var values = [];
    for (var i = 0; i < timeDomainData.length; i++) {
      values.push(timeDomainData[i]['value']);
    }
    var fft = new FFT(this.FFT_SAMPLE_SIZE, this.ASSUMED_SAMPLE_RATE);
    fft.forward(values);
    var spectrum = fft.spectrum;

    /*
    // Note:
    // According to https://github.com/corbanbrook/dsp.js/blob/master/dsp.js
    // which is the library we use to calculate FFT
    // the spectrum magnitudes are of the form
    // mag = bSi * sqrt(rval * rval + ival * ival);
    // where bSi is (2 / sampleSize)
    // Hence to get the actual microvolts value
    // to need to divide the spectrum magnitudes by bSi.
    for (var i=0; i<spectrum.length; i++) {
            spectrum[i] *= (1 / (2 / this.FFT_SAMPLE_SIZE));
    }
    */

    /*
    // Note:
    // To get the correct magnitudes, the spectrum values
    // need to be normalized by the number of frequency bins in the spectrum.
    // This number of bins is the the length of the spectrum array minus 1.
    for (var i=0; i<spectrum.length; i++) {
            spectrum[i] *= (1 / (spectrum.length - 1));
    }
    */

    return spectrum;
  }

  // TODO: might be able to use reduce
  // without creating a new array of values
  // That is, it might work to just reduce
  // against the timeDomainData object
  calculateChannelBaseline(channelIndex) {
    var timeDomainData = this.getTimeDomainData(channelIndex);
    if (timeDomainData.length < this.BASELINE_SAMPLE_SIZE) {
      return null;
    }

    var startIndex = timeDomainData.length - 1 - this.BASELINE_SAMPLE_SIZE;
    var stopIndex = timeDomainData.length - 1;
    var timeDomainData = timeDomainData.slice(startIndex, stopIndex);
    var values = [];
    for (var i = 0; i < timeDomainData.length; i++) {
      values.push(timeDomainData[i]['value']);
    }

    var average = values.reduce((a, b) => a + b) / values.length
    return average;
  }

  // Determine the max and min values of the given channel's recent data history
  calculateChannelValueBounds(channelIndex) {
    // Get the time domain data for the given channel
    var timeDomainData = this.getTimeDomainData(channelIndex);
    // If there is no data, then exit
    if (timeDomainData.length == 0) {
      return null;
    }
    // Use fancy javascript to calculate the bounds
    var valueBounds = {};
    valueBounds['min'] = timeDomainData.reduce(
        (min, p) => p.value < min ? p.value : min, timeDomainData[0].value);
    valueBounds['max'] = timeDomainData.reduce(
        (max, p) => p.value > max ? p.value : max, timeDomainData[0].value);
    return valueBounds;
  }

  // For the given channel
  // check to see if the channel value is holding constant and not zero.
  // If so, assume the channel is railing
  checkIfChannelIsRailing(channelIndex) {
    // If not enough samples have been collected
    if (this.getTimeDomainData(channelIndex).length <
        this.RAILING_MIN_SAMPLE_SIZE) {
      // Then assume no railing
      return false;
    }
    // Calculate the upper and lower bounds for this channel's data
    var valueBounds = this.calculateChannelValueBounds(channelIndex);
    // If bounds could not be calculated
    if (valueBounds == null) {
      // Then assume no railing
      return false;
    }
    // If the bounds are identical
    if (valueBounds['min'] == valueBounds['max']) {
      // If the bounds are not zero
      if (valueBounds['min'] != 0) {
        // Then assume railing
        return true;
      }
    }
    return false;
  }
}
