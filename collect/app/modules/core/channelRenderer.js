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

class ChannelRenderer {
  constructor(dataer, index, stage) {
    this.dataer = dataer;
    this.index = index;
    this.active = false;
    this.selected = false;
    // temporalPlotWidth should be = ((maxNumSamples - 1) * timeBetweenSamples)
    // otherwise plot line won't go all the way from right to left (in ms)
    this.temporalWidth = (this.dataer.getMaxNumSamplesPerChannel() - 1) * this.dataer.getAssumedTimeBetweenSamples();

    this.stage = stage;
    this.g = new PIXI.Graphics();
    this.stage.addChild(this.g);

    this.cursorTextStyle = new PIXI.TextStyle({
      fontFamily: 'Roboto',
      fontSize: 12,
      fill: '0xffffff',
    });
    this.cursorLabel = new PIXI.Text('', this.cursorTextStyle);
    this.g.addChild(this.cursorLabel);

    // used to render marks
    this.gm = new PIXI.Graphics(true);
    this.g.addChild(this.gm);
    this.marksLabelTextStyle = new PIXI.TextStyle({
      fontFamily: 'Roboto, Arial, sans-serif',
      fontSize: 12,
      fill: '0x000000',
    });
    this.marksLabels = [];

    this.LINE_WIDTH = 1;
    this.ARTIFACT_LINE_WIDTH = 3;
    this.DEFAULT_LINE_COLOR = 0x000000;
    this.SELECTED_LINE_COLOR = 0x00b190;
    this.RAILING_LINE_COLOR = 0xfc6868;
    this.ARTIFACT_LINE_COLOR = 0xedb90c;
    this.lineColor = this.DEFAULT_LINE_COLOR;

    this.mode = '';
    this.modeOptions = '';

    this.BASELINE_SAMPLE_SIZE = 500;
    // current baseline used for drawing
    this.currentBaseline = -1;
    this.currentBaselineTimeStamp = -1;
    // last sample drawn
    this.lastDrawnSample = null;
    // rolling baseline
    this.latestBaseline = -1;

    this.artifactPresent = false;
  }
  setIndex(index) {
    this.index = index;
  }
  setActive(active) {
    this.active = active;
    if (!this.active) {
      this.g.clear();
    }
  }
  setSelected(selected) {
    this.selected = selected;
  }
  setArtifacted(artifactPresent) {
    this.artifactPresent = artifactPresent;
  }
  updateDimensions(dims, viewportDims) {
    this.dims = dims;
    this.viewportDims = viewportDims;
  }
  setColor(color) {
    if (color) {
      this.lineColor = parseInt(color.substr(1), 16);
    } else {
      this.lineColor = this.DEFAULT_LINE_COLOR;
    }
  }
  // TODO: might be able to use reduce
  // without creating a new array of values
  // That is, it might work to just reduce
  // against the timeDomainData object
  calculateBaseline(data) {
    let values = [];
    for (let i = data.length - this.BASELINE_SAMPLE_SIZE; i < data.length; i++) {
      values.push(data[i]['value']);
    }
    let average = values.reduce((a, b) => a + b) / values.length;
    return average;
  }
  updateBaseline(baseline, timeStamp) {
    this.currentBaseline = baseline;
    this.currentBaselineTimeStamp = timeStamp;
    this.latestBaseline = baseline;
  }
  resetBaseline() {
    this.currentBaseline = -1;
    this.currentBaselineTimeStamp = -1;
    this.lastDrawnSample = null;
    this.latestBaseline = -1;
  }
  render(options, cursor, renderMarks) {
    if (!this.active) {
      return;
    }
    // reset drawing settings if drawing mode has changed
    if ((options.mode != this.mode) || (options.modeOptions != this.modeOptions)) {
      this.resetBaseline();
    }
    this.mode = options.mode;
    this.modeOptions = options.modeOptions;

    // clear screen
    this.g.clear();

    // change temporary line settings if artifact is present
    if (this.artifactPresent) {
      this.currentLineWidth = this.ARTIFACT_LINE_WIDTH;
      this.currentLineColor = this.ARTIFACT_LINE_COLOR;
    } else {
      this.currentLineWidth = this.LINE_WIDTH;
      this.currentLineColor = this.lineColor;
    }

    this.g.lineStyle(this.currentLineWidth, this.currentLineColor);
    if (renderMarks) {
      this.gm.clear();
      this.gm.lineStyle(1, 0x000000);
      for (let i = 0; i < this.marksLabels.length; i++) {
        if (this.marksLabels[i]) {
          this.marksLabels[i].visible = false;
        }
      }
    }
    this.cursorLabel.visible = false;

    if (this.dataer.checkIfChannelIsRailing(this.index)) {
      this.renderRailed();
    } else {
      if (options.mode == 'time') {
        if (options.modeOptions == 'autobaseline') {
          this.renderTimeDomainAutobaseline(options, cursor, renderMarks);
        } else if (options.modeOptions == 'autobaseline-continuous') {
          this.renderTimeDomainAutobaselineContinuous(options, cursor, renderMarks);
        } else {
          this.renderTimeDomain(options, cursor, renderMarks);
        }
      } else {
        this.renderFrequencyDomain(options, cursor);
      }
    }
  }
  renderRailed() {
    this.g.lineStyle(0);
    this.g.beginFill(0xdb4437);
    let y = this.dims.top + this.dims.height / 2;
    this.g.drawRect(this.dims.left, y - 2, this.dims.width, 4);
    this.g.endFill();
  }
  renderMarks(x, ts) {
    let marks = this.dataer.getMarkerData();
    for (let i = 0; i < marks.length; i++) {
      if (marks[i]['timeStamp'] == ts) {
        this.gm.moveTo(x, 0);
        this.gm.lineTo(x, this.viewportDims.height - this.viewportDims.marginBottom);
        if (i >= this.marksLabels.length) {
          this.marksLabels[i] = new PIXI.Text(marks[i]['value'], this.marksLabelTextStyle);
          this.g.addChild(this.marksLabels[i]);
        }
        this.marksLabels[i].text = marks[i]['value'];
        let bounds = this.marksLabels[i].getLocalBounds();
        this.marksLabels[i].x = x - bounds.width / 2;
        this.marksLabels[i].y = this.viewportDims.height - this.viewportDims.marginBottom + 2;
        this.marksLabels[i].visible = true;
      }
    }
  }
  renderCursor(cursorData) {
    if (cursorData) {
      this.cursorLabel.text = cursorData.label;

      let bounds = this.cursorLabel.getLocalBounds();
      let labelX, labelY;
      if ((this.mode == 'time') && (this.modeOptions != 'autobaseline')) {
        labelX = this.dims.left + this.dims.width - bounds.width - 10;
        labelY = 5;
      } else {
        labelX = cursorData.x + 2;
        labelY = cursorData.dataY - bounds.height / 2;
        if ((labelX + bounds.width + 6) > (this.dims.left + this.dims.width)) {
          labelX = cursorData.x - 2 - bounds.width - 6;
        }
        if (labelY < 0) {
          labelY = 0;
        } else if ((labelY + bounds.height + 6) > (this.dims.top + this.dims.height)) {
          labelY = this.dims.top + this.dims.height - bounds.height - 6;
        }
      }

      this.g.lineStyle(0);
      this.g.beginFill(this.SELECTED_LINE_COLOR, 1);
      this.g.moveTo(labelX, labelY);
      this.g.drawRoundedRect(labelX, labelY, bounds.width + 4, bounds.height + 4, 4);
      this.g.endFill();

      this.cursorLabel.x = labelX + 2;
      this.cursorLabel.y = labelY + 2;
      this.cursorLabel.visible = true;
    } else {
      this.cursorLabel.visible = false;
    }
  }
  renderTimeDomain(options, cursor, renderMarks) {
    // This function does not auto calculate the baseline voltage
    let data = this.dataer.getTimeDomainData(this.index);
    let maxUv = (options.yMax - options.yMin) / 2;
    let lastTimeStamp = data[data.length - 1]['timeStamp'];
    let started = false;
    let prevX = 0;
    let cursorData = null;
    for (let i = 0; i < data.length; i++) {
      let ts = data[i]['timeStamp'];
      if (lastTimeStamp - ts > this.temporalWidth) {
        continue;
      }
      let x = (1 - (lastTimeStamp - ts) / this.temporalWidth) * this.dims.width + this.dims.left;
      let y = (data[i]['value'] * -1) / maxUv * (this.dims.height / 2) + this.dims.height / 2 + this.dims.top;
      if (!started) {
        this.g.moveTo(x, y);
        started = true;
      } else {
        this.g.lineTo(x, y);
      }

      // A bit of a hack to draw marks here, because each channel calculates the x offset independently
      if (renderMarks) {
        this.renderMarks(x, ts);
      }

      if (cursor && this.selected) {
        if ((x == cursor.x) ||
            ((x >= cursor.x) && (i > 0) && (prevX < cursor.x))) {
          cursorData = {
            label: 'x = ' + ts + '\ny = ' + data[i]['value'],
            dataX: x,
            dataY: y,
            x: cursor.x,
            y: cursor.y,
          };
        }
      }
      prevX = x;
    }

    this.renderCursor(cursorData);
  }
  renderTimeDomainAutobaseline(options, cursor, renderMarks) {
    // This function recalculates the baseline every time the waveform fills
    // the width of the screen
    let data = this.dataer.getTimeDomainData(this.index);
    if (data.length < this.BASELINE_SAMPLE_SIZE) {
      return;
    }

    if (this.lastDrawnSample == null) {
      this.lastDrawnSample = data[data.length - this.BASELINE_SAMPLE_SIZE];
      this.updateBaseline(this.calculateBaseline(data), this.lastDrawnSample['timeStamp']);;
    } else {
      let index = data.lastIndexOf(this.lastDrawnSample);
      if (index == -1) {
        // The entire array contains new data, so recalculate the baseline and reset the view
        this.lastDrawnSample = data[data.length - this.BASELINE_SAMPLE_SIZE];
        this.updateBaseline(this.calculateBaseline(data), this.lastDrawnSample['timeStamp']);
      } else {
        // Keep track of rolling baseline, used in drawing below
        this.latestBaseline = this.calculateBaseline(data);
      }
    }

    let maxUv = (options.yMax - options.yMin) / 2;
    let prevX = 0;
    let cursorData = null;
    for (let i = 0; i < data.length; i++) {
      let ts = data[i]['timeStamp'];
      if (ts - this.currentBaselineTimeStamp < 0) {
        continue;
      }
      if ((ts - this.currentBaselineTimeStamp) > this.temporalWidth) {
        this.updateBaseline(this.latestBaseline, ts);
        this.g.clear();
        this.g.lineStyle(this.currentLineWidth, this.currentLineColor);
      }
      let x = (ts - this.currentBaselineTimeStamp) / this.temporalWidth * this.dims.width + this.dims.left;
      let y = ((data[i]['value'] - this.currentBaseline) * -1) / maxUv * (this.dims.height / 2) + this.dims.height / 2 + this.dims.top;
      if (x < 0) {
        continue;
      }
      if ((this.currentBaselineTimeStamp == ts) || (i == 0)) {
        this.g.moveTo(x, y);
      } else {
        this.g.lineTo(x, y);
      }

      // A bit of a hack to draw marks here, because each channel calculates the x offset independently
      if (renderMarks) {
        this.renderMarks(x, ts);
      }

      if (cursor && this.selected) {
        if ((x == cursor.x) ||
            ((x >= cursor.x) && (i > 0) && (prevX < cursor.x))) {
          cursorData = {
            label: 'x = ' + ts + '\ny = ' + data[i]['value'],
            dataX: x,
            dataY: y,
            x: cursor.x,
            y: cursor.y,
          };
        }
      }

      this.lastDrawnSample = data[i];
      prevX = x;
    }

    this.renderCursor(cursorData);
  }
  renderTimeDomainAutobaselineContinuous(options, cursor, renderMarks) {
    // This function recalculates the baseline every frame
    let data = this.dataer.getTimeDomainData(this.index);
    if (data.length < this.BASELINE_SAMPLE_SIZE) {
      return;
    }

    let baseline = this.calculateBaseline(data);
    let maxUv = (options.yMax - options.yMin) / 2;
    let lastTimeStamp = data[data.length - 1]['timeStamp'];
    let started = false;
    let prevX = 0;
    let cursorData = null;
    for (let i = 0; i < data.length; i++) {
      let ts = data[i]['timeStamp'];
      if (lastTimeStamp - ts > this.temporalWidth) {
        continue;
      }
      let x = (1 - (lastTimeStamp - ts) / this.temporalWidth) * this.dims.width + this.dims.left;
      let y = ((data[i]['value'] - baseline) * -1) / maxUv * (this.dims.height / 2) + this.dims.height / 2 + this.dims.top;
      if (!started) {
        this.g.moveTo(x, y);
        started = true;
      } else {
        this.g.lineTo(x, y);
      }

      // A bit of a hack to draw marks here, because each channel calculates the x offset independently
      if (renderMarks) {
        this.renderMarks(x, ts);
      }

      if (cursor && this.selected) {
        if ((x == cursor.x) ||
            ((x >= cursor.x) && (i > 0) && (prevX < cursor.x))) {
          cursorData = {
            label: 'x = ' + ts + '\ny = ' + data[i]['value'],
            dataX: x,
            dataY: y,
            x: cursor.x,
            y: cursor.y,
          };
        }
      }
      prevX = x;
    }

    this.renderCursor(cursorData);
  }
  renderFrequencyDomain(options, cursor) {
    let data = this.dataer.getFrequencyDomainData(this.index);
    if (!data) {
      return;
    }

    // Ignore the first bin, since it's the measure of total energy
    let binCount = data.length - 1;
    let prevX = 0;
    let cursorData = null;
    for (let i = 1; i < data.length; i++) {
      let x = ((i - 1) / binCount) * this.dims.width + this.dims.left;
      let y = (1 - (data[i] - options.yMin) / (options.yMax - options.yMin)) * this.dims.height + this.dims.top;
      if (i == 1) {
        this.g.moveTo(x, y);
      } else {
        this.g.lineTo(x, y);
      }

      if (cursor && this.selected) {
        if ((x == cursor.x) ||
            ((x >= cursor.x) && (i > 0) && (prevX < cursor.x))) {
          cursorData = {
            label: 'x = ' + (i - 1) + '\ny = ' + data[i],
            dataX: x,
            dataY: y,
            x: cursor.x,
            y: cursor.y,
          };
        }
      }
      prevX = x;
    }
    // draw the line to the end
    this.g.lineTo(this.dims.left + this.dims.width, this.dims.top + this.dims.height);

    this.renderCursor(cursorData);
  }
}
