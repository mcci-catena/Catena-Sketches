# Understanding MCCI Catena data format 0x16

<!-- TOC depthFrom:2 updateOnSave:true -->

- [Overall Message Format](#overall-message-format)
- [Bitmap fields and associated fields](#bitmap-fields-and-associated-fields)
	- [Battery Voltage (field 0)](#battery-voltage-field-0)
	- [Bus Voltage (field 1)](#bus-voltage-field-1)
	- [Boot counter (field 2)](#boot-counter-field-2)
	- [Environmental Readings (field 3)](#environmental-readings-field-3)
	- [Lux (field 4)](#lux-field-4)
	- [Water Pressure and Water Level (field 5)](#water-pressure-and-water-level-field-5)
- [Data Formats](#data-formats)
	- [uint16](#uint16)
	- [int16](#int16)
	- [uint8](#uint8)
	- [uflt16](#uflt16)
- [Test Vectors](#test-vectors)
- [Node-RED Decoding Script](#node-red-decoding-script)
- [The Things Network Console decoding script](#the-things-network-console-decoding-script)

<!-- /TOC -->

## Overall Message Format

MCCI Catena format 0x16 messages are always sent on LoRaWAN port 1. Each message has the following layout.

byte | description
:---:|:---
0 | Format code (always 0x16, decimal 22).
1 | bitmap encoding the fields that follow
2..n | data bytes; use bitmap to decode.

Each bit in byte 1 represent whether a corresponding field in bytes 2..n is present. If all bits are clear, then no data bytes are present. If bit 0 is set, then field 0 is present; if bit 1 is set, then field 1 is present, and so forth. Fields, if present, are

## Bitmap fields and associated fields

The bitmap byte has the following interpretation. `int16`, `uint16`, etc. are defined after the table.

Bitmap bit | Length of corresponding field (bytes) | Data format |Description
:---:|:---:|:---:|:----
0 | 2 | [int16](#int16) | [Battery voltage](#battery-voltage-field-0)
1 | 2 | [int16](#int16) | [Bus voltage](#bus-voltage-field-1)
2 | 1 | [uint8](#uint8) | [Boot counter](#boot-counter-field-2)
3 | 5 | [int16](#int16), [uint16](#uint16), [uint8](#uint8) | [Temperature, pressure, humidity](environmental-readings-field-3)
4 | 2 | [uint16](#uint16) | [Ambient Light](#lux-field-4)
5 | 2 | [uflt16](#uflt16) | [Water Pressure](#water-pressure-and-water-level-field-5)
6..7 | n/a | n/a | These two bits are reserved. They must always be zero.

### Battery Voltage (field 0)

Field 0, if present, carries the current battery voltage. To get the voltage, extract the int16 value, and divide by 4096.0. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

### Bus Voltage (field 1)

Field 1, if present, carries the current Vbus voltage. Divide by 4096.0 to convert from counts to volts. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

_Note:_ this field is not transmitted by V1.1 of the `catena_aqi.ino` sketch.

### Boot counter (field 2)

Field 2, if present, is a counter of number of recorded system reboots, modulo 256.

### Environmental Readings (field 3)

Field 3, if present, has three environmental readings.

- The first two bytes are a [`int16`](#int16) representing the temperature (divide by 256 to get degrees C).

- The next two bytes are a [`uint16`](#uint16) representing the barometric pressure (divide by 25 to get millibars). This is the station pressure, not the sea-level pressure.

- The last byte is a [`uint8`](#uint8) representing the relative humidity (divide by 2.56 to get percent).  (This field can represent humidity from 0% to 99.6%.)

### Lux (field 4)

Field 4, if present, directly represents the ambient light level in lux as a [`uint16`](#uint16). The measurement ranges from 0 to 65536.

### Water Pressure and Water Level (field 5)

Field 5, if present, represents the Water Pressure as recorded by the RAYCO RC601 water pressure transducer.
Using the recorded water pressure values, the water level can also be measured. This is achieved by using the formula as follows:

				Water Level = (Water Pressure) /9.81


## Data Formats

All multi-byte data is transmitted with the most significant byte first (big-endian format).  Comments on the individual formats follow.

### uint16

an integer from 0 to 65536.

### int16

a signed integer from -32,768 to 32,767, in two's complement form. (Thus 0..0x7FFF represent 0 to 32,767; 0x8000 to 0xFFFF represent -32,768 to -1).

### uint8

an integer from 0 to 255.

### uflt16

A unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
15..12 | binary exponent `b`
11..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/4096 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

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

|Input | vBat | vBus | Boot | Temp(deg C) | P (mBar) | RH % | Lux |  Water Pressure  |
|:-----|-----:|-----:|-----:|-------------:|---------:|-----:|----:|------:|
|`17 01 18 00` | +1.5 | | |  |            |           |      |     |       |
|`17 01 F8 00` | -0.5 | | |  |            |           |      |     |       |
|`17 05 F8 00 42` | -0.5 |  | 66 |            |           |      |     |       |
|`16 3D 46 F4 59 1E CB 62 9F 68 00 F7 67 85` | 4.434 | 4.938 | 89 | 30.792 | 1009.88 | 40.625 | 247 | 10.805 |

## Node-RED Decoding Script

The following Node-RED script will decode this data. You can download the latest version from github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/master/extra/catena-message-0x17-decoder-node-red.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-0x14-decoder-node-red.js

```javascript
// JavaScript source code
// This Node-RED decoding function decodes the record sent by the Catena 4450
// Water-Pressure & Water-Level Sensing application.

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
// result.wPressure: water pressure value in Kpa.
// result.wLevel: Water Level value.
var result = {};

// check the message type byte
if (b[0] != 0x16) {
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


if (flags & 0x20)
{
	//Decode Rayco Sensor Data for Water Pressure & Water Level.
	var ρ = 1000; 
    var g = 9.81;
    var wpRaw;
	var wpFinal;
    wpRaw  = (b[i] << 8) + b[i + 1];
    i += 2;
    //Convert pressure from hPa to kPa and store payload.
    wpFinal = (wpRaw* 4 / 100.0) / 10;
	result.wPressure = wpFinal;
	result.wLevel = ((wpFinal)*1000) / (ρ*g);
}

// now update msg with the new payload and new .local field
// the old msg.payload is overwritten.
msg.payload = result;
msg.local =
    {
        nodeType: "Catena 4450",
        platformType: "Feather M0 LoRa",
        radioType: "RF95",
        applicationName: "Water-Level Sensor"
    };

return msg;

```

## The Things Network Console decoding script

The following script decodes format 0x16 for [The Things Network console](https://console.thethingsnetwork.org). This script also supports formats 0x11, 0x14, and 0x15.
You can get the latest version on github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/master/extra/catena-message-generic-decoder-ttn.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-generic-decoder-ttn.js

```javascript
// This function decodes the records (port 1, format 0x11, 0x14, 0x15, 0x16, 0x17)
// sent by the MCCI Catena 4410/4450/4551 soil/water and power applications.
// For use with console.thethingsnetwork.org
// 2017-09-19 add dewpoints.
// 2017-12-13 fix commments, fix negative soil/water temp, add test vectors.
// 2017-12-15 add format 0x11.
// 2018-04-24 add format 0x16.

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

//Calculate Water Level.

function waterlevel(wp) {
    var ρ = 1000; 
    var g = 9.81;
    var h = (wp*1000) / (ρ*g);
    return h;
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
			//	{
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
			//	{
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
		} else if (cmd == 0x16) {
			
			// decode Catena 4450 Water Level data
			
			// test vectors:
			//  17 01 18 00 ==> vBat = 1.5
			//  17 01 F8 00 ==> vBat = -0.5
			//  17 05 F8 00 42 ==> boot: 66, vBat: -0.5
			//  17 0D F8 00 42 17 80 59 35 80 ==> adds one temp of 23.5, rh = 50, p = 913.48
			//  16 3D 46 F4 59 1E CB 62 9F 68 00 F7 67 85 ==>
			//	{
			//    "water pressure": 01.805
			//    "boot": 89,
			//    "error": "none",
			//    "lux": 247,
			//    "p": 1009.88,
			//    "rh": 40.625,
			//    "tempC": 30.792,
			//    "vBat": 4.434,
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
				// scale and save.
				decoded.vBat = vRaw / 4096.0;
			}

			if (flags & 0x2) {
				var vRaw = (bytes[i] << 8) + bytes[i + 1];
				i += 2;
				if (vRaw & 0x8000)
					vRaw += -0x10000;
				decoded.vBus = vRaw / 4096.0;
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

				decoded.t = tRaw / 256;
				decoded.p = pRaw * 4 / 100.0;
				decoded.rh = hRaw / 256 * 100;
			}

			if (flags & 0x10) {
				// we have lux
				var luxRaw = (bytes[i] << 8) + bytes[i + 1];
				i += 2;
				decoded.lux = luxRaw;
			}


			if (flags & 0x20)
			{
				//Decode Rayco Sensor Data.
				var wpRaw;
				wpRaw  = (bytes[i] << 8) + bytes[i + 1];
				i += 2;
				//Convert pressure from hPa to kPa. And display payload.
				decoded.wPressure = (wpRaw* 4 / 100.0) / 10;
				decoded.wLevel = waterlevel(decoded.wPressure);
			}
			
		} else if (cmd == 0x17) {
			// decode Catena 4460 AQI data
			
			// test vectors:
			//  17 01 18 00 ==> vBat = 1.5
			//  17 01 F8 00 ==> vBat = -0.5
			//  17 05 F8 00 42 ==> boot: 66, vBat: -0.5
			//  17 0D F8 00 42 17 80 59 35 80 ==> adds one temp of 23.5, rh = 50, p = 913.48, tDewC = 12.5
			//  17 3D 44 60 0D 15 9D 5F CD C3 00 00 F9 07 ==>
			//	{
			//    "aqi": 288.875
			//    "boot": 13,
			//    "error": "none",
			//    "lux": 0,
			//    "p": 981,
			//    "rh": 76.171875,
			//    "tDewC": 17.236466758309017,
			//    "tempC": 21.61328125,
			//    "vBat": 4.2734375,
			//    }
			// 17 3D 43 72 07 17 A4 5F CB A7 01 DB E9 AF ==>
			//    {
			//    "aqi": 154.9375,
			//    "boot": 7,
			//    "error": "none",
			//    "lux": 475,
			//    "p": 980.92,
			//    "rh": 65.234375,
			//    "tDewC": 16.732001483771757,
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
				var rawUflt16 = (bytes[i] << 8) + bytes[i + 1];
				i += 2;
				
				var exp1 = rawUflt16 >> 12;
				var mant1 = (rawUflt16 & 0xFFF) / 4096.0;
				var f_unscaled = mant1 * Math.pow(2, exp1 - 15);
				var aqi = f_unscaled * 512;
				
				decoded.aqi = aqi;
			}
		} else {
			// cmd value not recognized.
		}
	}

	// at this point, decoded has the real values.
	return decoded;
}
```
