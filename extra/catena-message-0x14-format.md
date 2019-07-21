# Understanding Catena Message Format 0x14

## Overall Message Format

MCCI Catena format 0x14 messages are always sent on LoRaWAN port 1. Each message has the following layout.

byte | description
:---:|:---
0 | Format code (always 0x14, decimal 20).
1 | bitmap encoding the fields that follow
2..n | data bytes; use bitmap to decode.

Each bit in byte 1 represent whether a corresponding field in bytes 2..n is present. If all bits are clear, then no data bytes are present. If bit 0 is set, then field 0 is present; if bit 1 is set, then field 1 is present, and so forth.

Fields are appended sequentially in ascending order.  A bitmap of 0000101 indicates that field 0 is present, followed by field 2; the other fields are missing.  A bitmap of 00011010 indicates that fields 1, 3, and 4 are present, in that order, but that fields 0, 2, 5 and 6 are missing.

## Field format definitions

Each field has its own format, as defined in the following table. `int16`, `uint16`, etc. are defined after the table.

field number | length of field| encoding | description
:---:|:---:|:---:|:---
0 | 2 | int16 | battery voltage; divide by 4096 to convert from counts to volts.
1 | 2 | int16 | bus voltage; divide by 4096 to convert from counts to volts. _Note:_ this field is currently never transmitted.
2 | 1 | uint8 | counter of number of recorded system reboots, modulo 256.
3 | 5 | int16, uint16, uint8 | Temperature (divide by 256 to get degrees C); pressure (divide by 25 to get millibars); relative humidity (divide by 2.56 to get percent).
4 | 2 | uint16 | Lux, from 0 to 65536.
5 | 4 | uint16, uint16 | Watt-hour pulses since reset, modulo 65,536. First value is for power consumed from the grid, second for power sourced to the grid. This must be scaled according to the CT and watt-hour meter used in order to convert to engineering units. Test vectors assume multiply by 10 to get watt-hours.
6 | 4 | uflt16, uflt16 | Instantaneous power over this measurement period. First value is for power consumed, second is for power produced.  This must be scaled according to the CT and watt-hour meter used in order to convert to engineering units. Test vectors assume multiply by 10 to get watts.
7 | n/a | n/a | reserved, must always be zero.

## Data Formats

All multi-byte data is transmitted with the most significant byte first (big-endian format).  Comments on the individual formats follow.

### uint16

an integer from 0 to 65536.

### int16

a signed integer from -32,768 to 32,767, in two's complement form. (Thus 0..0x7FFF represent 0 to 32,767; 0x8000 to 0xFFFF represent -32,768 to -1).

### uint8

an integer from 0 to 255.

### uflt16

A unsigned floating point number, transmitted as a 16-bit number, with the following interpretation:

bits | description
:---:|:---
15..12 | binary exponent `b`
11..0 | fraction `f`

The floating point number is computed by computing `f`/4096 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0x1A, 0xAB, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x1AAB.
2. `b`  is therefore 0x1, and `b`-15 is -14.  2^-14 is 1/32768
3. `f` is 0xAAB. 0xAAB/4096 is 0.667
4. `f * 2^(b-15)` is therefore 0.6667/32768 or 0.0000204

Floating point mavens will immediately recognize:

* There is no sign bit; all numbers are positive.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 2048.. 4095, and then transmit only `f - 2048`, saving a bit. However, this complicated the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

## Test Vectors

The following input data can be used to test decoders.

|Input | vBat | vBus | Boot | Temp (deg C) | P (mBar) | RH % | Lux | wH in | wH out | w In | w Out |
|:-----|-----:|-----:|-----:|-------------:|---------:|-----:|----:|------:|-------:|-----:|------:|
|`14 01 18 00` | +1.5 | | |  |            |           |      |     |       |        |      |     |
|`14 01 F8 00` | -0.5 | | |  |            |           |      |     |       |        |      |      |
|`14 05 F8 00 42` | -0.5 |  | 66 |            |           |      |     |       |        |      |    |
|`14 7D 43 A7 2B 19 8D 5F 88 8E 00 2E 00 00 00 00 00 00 00 00` | 4.229 | |  43 | 25.550 | 978.24 | 55.5 | 46 | 0 | 0 | 0 | 0 |
|`14 7D 43 23 11 19 52 5F 97 AE 00 00 C5 3F 00 00 BF 9E 00 00` | 4.196 | |  17 | 25.320 | 978.84 | 68.0 | 0 | 50,495 | 0 | 878.47 | 0 |
|`14 7F 43 23 4F 01 11 19 52 5F 97 AE 03 01 C5 50 31 24 BF 54 D8 39` | 4.196 | 4.938 | 17 | 25.320 | 978.84 | 68.0 | 769 | 50,512 | 12,580 | 862.21 | 1850.1 |

## Node-RED Decoding Script

The following Node-RED script will decode this data. You can download the latest version from github [in raw form](https://raw.githubusercontent.com/mcci-catena/Catena4410-Sketches/master/extra/catena-message-0x14-decoder-node-red.js) or [view it](https://github.com/mcci-catena/Catena4410-Sketches/blob/master/extra/catena-message-0x14-decoder-node-red.js) on github.

```javascript
// JavaScript source code
// This Node-RED decoding function decodes the record sent by the Catena 4450
// M101 power monitor application.
// Written in a big hurry, so no points for style

var b = msg.payload;  // pick up data for convenience; just saves typing.

// an empty table to which we'll add result fields:
//
// result.vBat: the battery voltage (if present)
// result.vBus: the USB charger voltage (if provided)
// result.boot: the system boot counter, modulo 256
// result.t: temperature in degrees C
// result.p: station pressure in hPa (millibars). Note that this is not
//   adjusted for the height above sealevel so can't be directly compared
//   to weather.gov "barometric pressure"
// result.rh: relative humidity (in %)
// result.lux: light level, in lux
// result.powerUsed: pulses from the WH pulse meter (consumption)
// result.powerRev: pulese from the WH pulse meter (sourced to the grid)
// result.powerUsedDeriv: derivative of total power used over the last
//    sample period, normalised to kWh / hour (in other words, kW).
// result.powerRevDeriv: derivative of total power sourced over the
//    last sample period, normalized to kWh/hour.
var result = {};

// check the message type byte
if (b[0] != 0x14) {
    // not one of ours: report an error, return without a value,
    // so that Node-RED doesn't propagate the message any further.
    node.error("not ours! " + b[0].toString());
    return;
}

// i is used as the index into the message. Start with the flag byte.
var i = 1;
// fetch the bitmap.
var flags = b[i++];

if (flags & 0x1) {
    // set vRaw to a uint16, and increment pointer
    var vRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    // interpret uint16 as an int16 instead.
    if (vRaw & 0x8000)
        vRaw += -0x10000;
    // scale and save in result.
    result.vBat = vRaw / 4096.0;
}

if (flags & 0x2) {
    var vRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    if (vRaw & 0x8000)
        vRaw += -0x10000;
    result.vBus = vRaw / 4096.0;
}

if (flags & 0x4) {
    var iBoot = b[i];
    i += 1;
    result.boot = iBoot;
}

if (flags & 0x8) {
    // we have temp, pressure, RH
    var tRaw = (b[i] << 8) + b[i + 1];
    if (tRaw & 0x8000)
        tRaw = -0x10000 + tRaw;
    i += 2;
    var pRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    var hRaw = b[i++];

    result.t = tRaw / 256;
    result.p = pRaw * 4 / 100.0;
    result.rh = hRaw / 256 * 100;
}

if (flags & 0x10) {
    // we have lux
    var luxRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    result.lux = luxRaw;
}


if (flags & 0x20)   // watthour
{
    var powerIn = (b[i] << 8) + b[i + 1];
    i += 2;
    var powerOut = (b[i] << 8) + b[i + 1];
    i += 2;
    result.powerUsed = powerIn;
    result.powerRev = powerOut;
}

if (flags & 0x40)  // normalize floating pulses per hour
{
    var floatIn = (b[i] << 8) + b[i + 1];
    i += 2;
    var floatOut = (b[i] << 8) + b[i + 1];
    i += 2;

    var exp1 = floatIn >> 12;
    var exp2 = floatOut >> 12;
    var mant1 = (floatIn & 0xFFF) / 4096.0;
    var mant2 = (floatOut & 0xFFF) / 4096.0;
    var powerPerHourIn = mant1 * Math.pow(2, exp1 - 15) * 60 * 60 * 4;
    var powerPerHourOut = mant2 * Math.pow(2, exp2 - 15) * 60 * 60 * 4;
    result.powerUsedDeriv = powerPerHourIn;
    result.powerRevDeriv = powerPerHourOut;
}

// now update msg with the new payload and new .local field
// the old msg.payload is overwritten.
msg.payload = result;
msg.local =
    {
        nodeType: "Catena 4450-M101",
        platformType: "Feather M0 LoRa",
        radioType: "RF95",
        applicationName: "AC Power Monitoring"
    };

return msg;
```

## The Things Network Console decoding script

The following script does similar decoding for [The Things Network console](https://console.thethingsnetwork.org). This script also supports formats 0x11 and 0x15.

```javascript
// This function decodes the records (port 1, format 0x11, 0x14, 0x15)
// sent by the MCCI Catena 4410/4450/4551 soil/water and power applications.
// For use with console.thethingsnetwork.org
// 2017-09-19 add dewpoints.
// 2017-12-13 fix commments, fix negative soil/water temp, add test vectors.
// 2017-12-15 add format 0x11.

// calculate dewpoint (degrees C) given temperature (C) and relative humidity (0..100)
// from http://andrew.rsmas.miami.edu/bmcnoldy/Humidity.html
// rearranged for efficiency and to deal sanely with very low (< 1%) RH
function dewpoint(t, rh) {
    var c1 = 243.04;
    var c2 = 17.625;
    var h = rh / 100;
    if (h <= 0.01)
        h = 0.01;
    else if (h > 1.0)
        h = 1.0;

    var lnh = Math.log(h);
    var tpc1 = t + c1;
    var txc2 = t * c2;
    var txc2_tpc1 = txc2 / tpc1;

    var tdew = c1 * (lnh + txc2_tpc1) / (c2 - lnh - txc2_tpc1);
    return tdew;
}

function Decoder(bytes, port) {
    // Decode an uplink message from a buffer
    // (array) of bytes to an object of fields.
    var decoded = {};

    if (port === 1) {
        cmd = bytes[0];
        if (cmd == 0x14) {
            // decode Catena 4450 M101 data

            // test vectors:
            //  14 01 18 00 ==> vBat = 1.5
            //  14 01 F8 00 ==> vBat = -0.5
            //  14 05 F8 00 42 ==> boot: 66, vBat: -0.5
            //  14 0D F8 00 42 17 80 59 35 80 ==> adds one temp of 23.5, rh = 50, p = 913.48

            // i is used as the index into the message. Start with the flag byte.
            var i = 1;
            // fetch the bitmap.
            var flags = bytes[i++];

            if (flags & 0x1) {
                // set vRaw to a uint16, and increment pointer
                var vRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                // interpret uint16 as an int16 instead.
                if (vRaw & 0x8000)
                    vRaw += -0x10000;
                // scale and save in decoded.
                decoded.vBat = vRaw / 4096.0;
            }

            if (flags & 0x2) {
                var vRawBus = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                if (vRawBus & 0x8000)
                    vRawBus += -0x10000;
                decoded.vBus = vRawBus / 4096.0;
            }

            if (flags & 0x4) {
                var iBoot = bytes[i];
                i += 1;
                decoded.boot = iBoot;
            }

            if (flags & 0x8) {
                // we have temp, pressure, RH
                var tRaw = (bytes[i] << 8) + bytes[i + 1];
                if (tRaw & 0x8000)
                    tRaw = -0x10000 + tRaw;
                i += 2;
                var pRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                var hRaw = bytes[i++];

                decoded.tempC = tRaw / 256;
                decoded.error = "none";
                decoded.p = pRaw * 4 / 100.0;
                decoded.rh = hRaw / 256 * 100;
                decoded.tDewC = dewpoint(decoded.tempC, decoded.rh);
            }

            if (flags & 0x10) {
                // we have lux
                var luxRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                decoded.lux = luxRaw;
            }

            if (flags & 0x20) {
                // watthour
                var powerIn = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                var powerOut = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                decoded.powerUsedCount = powerIn;
                decoded.powerSourcedCount = powerOut;
            }

            if (flags & 0x40) {
                // normalize floating pulses per hour
                var floatIn = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                var floatOut = (bytes[i] << 8) + bytes[i + 1];
                i += 2;

                var exp1 = floatIn >> 12;
                var exp2 = floatOut >> 12;
                var mant1 = (floatIn & 0xFFF) / 4096.0;
                var mant2 = (floatOut & 0xFFF) / 4096.0;
                var powerPerHourIn = mant1 * Math.pow(2, exp1 - 15) * 60 * 60 * 4;
                var powerPerHourOut = mant2 * Math.pow(2, exp2 - 15) * 60 * 60 * 4;
                decoded.powerUsedPerHour = powerPerHourIn;
                decoded.powerSourcedPerHour = powerPerHourOut;
            }
        } else if (cmd == 0x15) {
            // decode Catena 4450 M102 data

            // test vectors:
            //  15 01 18 00 ==> vBat = 1.5
            //  15 01 F8 00 ==> vBat = -0.5
            //  15 05 F8 00 42 ==> boot: 66, vBat: -0.5
            //  15 0D F8 00 42 17 80 59 35 80 ==> adds one temp of 23.5, rh = 50, p = 913.48, tDewC = 12.5
            //  15 7D 44 60 0D 15 9D 5F CD C3 00 00 1C 11 14 46 E4 ==>
            //    {
            //    "boot": 13,
            //    "error": "none",
            //    "lux": 0,
            //    "p": 981,
            //    "rh": 76.171875,
            //    "rhSoil": 89.0625,
            //    "tDewC": 17.236466758309017,
            //    "tSoil": 20.2734375,
            //    "tSoilDew": 18.411840342527178,
            //    "tWater": 28.06640625,
            //    "tempC": 21.61328125,
            //    "vBat": 4.2734375,
            //    }
            // 15 7D 43 72 07 17 A4 5F CB A7 01 DB 1C 01 16 AF C3
            //    {
            //    "boot": 7,
            //    "error": "none",
            //    "lux": 475,
            //    "p": 980.92,
            //    "rh": 65.234375,
            //    "rhSoil": 76.171875,
            //    "tDewC": 16.732001483771757,
            //    "tSoil": 22.68359375,
            //    "tSoilDew": 18.271601276518467,
            //    "tWater": 28.00390625,
            //    "tempC": 23.640625,
            //    "vBat": 4.21533203125
            //    }
            // 15 7D 42 D4 21 F5 9B 5E 5F C1 00 00 01 C1 F9 1B EC
            //    {
            //    "boot": 33,
            //    "error": "none",
            //    "lux": 0,
            //    "p": 966.36,
            //    "rh": 75.390625,
            //    "rhSoil": 92.1875,
            //    "tDewC": -13.909882718758952,
            //    "tSoil": -6.89453125,
            //    "tSoilDew": -7.948780789914008,
            //    "tWater": 1.75390625,
            //    "tempC": -10.39453125,
            //    "vBat": 4.1767578125
            //    }
            // i is used as the index into the message. Start with the flag byte.
            var i = 1;
            // fetch the bitmap.
            var flags = bytes[i++];

            if (flags & 0x1) {
                // set vRaw to a uint16, and increment pointer
                var vRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                // interpret uint16 as an int16 instead.
                if (vRaw & 0x8000)
                    vRaw += -0x10000;
                // scale and save in decoded.
                decoded.vBat = vRaw / 4096.0;
            }

            if (flags & 0x2) {
                var vRawBus = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                if (vRawBus & 0x8000)
                    vRawBus += -0x10000;
                decoded.vBus = vRawBus / 4096.0;
            }

            if (flags & 0x4) {
                var iBoot = bytes[i];
                i += 1;
                decoded.boot = iBoot;
            }

            if (flags & 0x8) {
                // we have temp, pressure, RH
                var tRaw = (bytes[i] << 8) + bytes[i + 1];
                if (tRaw & 0x8000)
                    tRaw = -0x10000 + tRaw;
                i += 2;
                var pRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                var hRaw = bytes[i++];

                decoded.tempC = tRaw / 256;
                decoded.error = "none";
                decoded.p = pRaw * 4 / 100.0;
                decoded.rh = hRaw / 256 * 100;
                decoded.tDewC = dewpoint(decoded.tempC, decoded.rh);
            }

            if (flags & 0x10) {
                // we have lux
                var luxRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                decoded.lux = luxRaw;
            }

            if (flags & 0x20) {
                // onewire temperature
                var tempRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                if (tempRaw & 0x8000)
                    tempRaw = -0x10000 + tempRaw;
                decoded.tWater = tempRaw / 256;
            }

            if (flags & 0x40) {
                // temperature followed by RH
                var tempRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                if (tempRaw & 0x8000)
                    tempRaw = -0x10000 + tempRaw;
                var tempRH = bytes[i];
                i += 1;
                decoded.tSoil = tempRaw / 256;
                decoded.rhSoil = tempRH / 256 * 100;
                decoded.tSoilDew = dewpoint(decoded.tSoil, decoded.rhSoil);
            }
        } else if (cmd == 0x11) {
            // decode Catena 4410 sensor data

            // test vectors:
            //  11 01 18 00 ==> vBat = 1.5
            //  11 01 F8 00 ==> vBat = -0.5
            //  11 05 F8 00 17 80 59 35 80 ==> adds one temp of 23.5, rh = 50, p = 913.48, tDewC = 12.5
            //  11 3D 44 60 15 9D 5F CD C3 00 00 1C 11 14 46 E4 ==>
            //    {
            //    "error": "none",
            //    "lux": 0,
            //    "p": 981,
            //    "rh": 76.171875,
            //    "rhSoil": 89.0625,
            //    "tDewC": 17.236466758309017,
            //    "tSoil": 20.2734375,
            //    "tSoilDew": 18.411840342527178,
            //    "tWater": 28.06640625,
            //    "tempC": 21.61328125,
            //    "vBat": 4.2734375,
            //    }
            // 11 3D 43 72 17 A4 5F CB A7 01 DB 1C 01 16 AF C3
            //    {
            //    "error": "none",
            //    "lux": 475,
            //    "p": 980.92,
            //    "rh": 65.234375,
            //    "rhSoil": 76.171875,
            //    "tDewC": 16.732001483771757,
            //    "tSoil": 22.68359375,
            //    "tSoilDew": 18.271601276518467,
            //    "tWater": 28.00390625,
            //    "tempC": 23.640625,
            //    "vBat": 4.21533203125
            //    }
            // i is used as the index into the message. Start with the flag byte.
            var i = 1;
            // fetch the bitmap.
            var flags = bytes[i++];

            if (flags & 0x1) {
                // set vRaw to a uint16, and increment pointer
                var vRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                // interpret uint16 as an int16 instead.
                if (vRaw & 0x8000)
                    vRaw += -0x10000;
                // scale and save in decoded.
                decoded.vBat = vRaw / 4096.0;
            }

            if (flags & 0x2) {
                var vRawBus = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                if (vRawBus & 0x8000)
                    vRawBus += -0x10000;
                decoded.vBus = vRawBus / 4096.0;
            }

            if (flags & 0x4) {
                // we have temp, pressure, RH
                var tRaw = (bytes[i] << 8) + bytes[i + 1];
                if (tRaw & 0x8000)
                    tRaw = -0x10000 + tRaw;
                i += 2;
                var pRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                var hRaw = bytes[i++];

                decoded.tempC = tRaw / 256;
                decoded.error = "none";
                decoded.p = pRaw * 4 / 100.0;
                decoded.rh = hRaw / 256 * 100;
                decoded.tDewC = dewpoint(decoded.tempC, decoded.rh);
            }

            if (flags & 0x8) {
                // we have lux
                var luxRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                decoded.lux = luxRaw;
            }

            if (flags & 0x10) {
                // onewire temperature
                var tempRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                decoded.tWater = tempRaw / 256;
            }

            if (flags & 0x20) {
                // temperature followed by RH
                var tempRaw = (bytes[i] << 8) + bytes[i + 1];
                i += 2;
                var tempRH = bytes[i];
                i += 1;
                decoded.tSoil = tempRaw / 256;
                decoded.rhSoil = tempRH / 256 * 100;
                decoded.tSoilDew = dewpoint(decoded.tSoil, decoded.rhSoil);
            }
        } else {
            // nothing
        }
    }
    return decoded;
}
```
