# Recording format

The recording output file has a header containing a JSON blob, followed by a
separator (`<CR><LF>----<CR><LF>)`, followed by rows of comma-separated values.
The fields in each row of data are described by the JSON header.

## Header format

The schema for the header is [defined here](recording.json).

## Example

For a recording with 3 channels, labeled `Fz`, `Cz` and `Pz`, data in raw ADC
output, channel gain 24 and one column of mark data:

```
{
  channelCount: 3,
  channels: [{
    gain: 24,
    index: 1,
    isOn: true,
    label: "Fz",
  }, {
    gain: 24,
    index: 2,
    isOn: true,
    label: "Cz",
  }, {
    gain: 24,
    index: 3,
    isOn: true,
    label: "Pz",
  }],
  timestampIndex: 0,
  markIndex: 3,
  unit: "adc",
  collectVersion: "1.0.0",
  bioampId: "Luchador 3.0",
  bioampVersion: "1.0.0",
}
----
39514,-1570,-5062,-1234,0
39515,-1702,-5329,-3425,0
39516,-1560,-5006,-4545,0
<...more rows...>
```
