# Understanding MCCI Catena data format 0x15

<!-- TOC depthFrom:2 updateOnSave:true -->

- [Overall Message Format](#overall-message-format)
- [Field format definitions](#field-format-definitions)
	- [Battery Voltage (field 0)](#battery-voltage-field-0)
	- [Bus Voltage (field 1)](#bus-voltage-field-1)
	- [Boot counter (field 2)](#boot-counter-field-2)
	- [Environmental Readings (field 3)](#environmental-readings-field-3)
	- [Ambient light (field 4)](#ambient-light-field-4)
	- [Temperature Probe (field 5)](#temperature-probe-field-5)
	- [Soil probe (field 6)](#soil-probe-field-6)
- [Data Formats](#data-formats)
	- [uint16](#uint16)
	- [int16](#int16)
	- [uint8](#uint8)
- [Test Vectors](#test-vectors)
- [Node-RED Decoding Script](#node-red-decoding-script)
- [The Things Network Console decoding script](#the-things-network-console-decoding-script)

<!-- /TOC -->

## Overall Message Format

MCCI Catena format 0x15 messages are always sent on LoRaWAN port 1. Each message has the following layout.

byte | description
:---:|:---
0 | Format code (always 0x15, decimal 21).
1 | bitmap encoding the fields that follow
2..n | data bytes; use bitmap to decode.

Each bit in byte 1 represent whether a corresponding field in bytes 2..n is present. If all bits are clear, then no data bytes are present. If bit 0 is set, then field 0 is present; if bit 1 is set, then field 1 is present, and so forth.

Fields are appended sequentially in ascending order.  A bitmap of 0000101 indicates that field 0 is present, followed by field 2; the other fields are missing.  A bitmap of 00011010 indicates that fields 1, 3, and 4 are present, in that order, but that fields 0, 2, 5 and 6 are missing.

## Field format definitions

Each field has its own format, as defined in the following table. `int16`, `uint16`, etc. are defined after the table.

Field number (Bitmap bit) | Length of corresponding field (bytes) | Data format |Description
:---:|:---:|:---:|:----
0 | 2 | [int16](#int16) | [Battery voltage](#battery-voltage-field-0)
1 | 2 | [int16](#int16) | [Bus voltage](#bus-voltage-field-1)
2 | 1 | [uint8](#uint8) | [Boot counter](#boot-counter-field-2)
3 | 5 | [int16](#int16), [uint16](#uint16), [uint8](#uint8) | [Temperature, pressure, humidity](environmental-readings-field-3)
4 | 2 | [uint16](#uint16) | [Ambient Light](#ambient-light-field-4)
5 | 2 | [int16](#int16) | [Temperature Probe](#temperature-probe-field-5)
6 | 2 | [int16](#int16), [uint8](#uint8) | [Soil temperature/humidity probe](#soil-probe-field-6)
7 | n/a | n/a | reserved, must always be zero.

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

### Ambient light (field 4)

Field 4, if present, directly represents the ambient light level in lux as a [`uint16`](#uint16). The measurement ranges from 0 to 65536.

### Temperature Probe (field 5)

Field 5, if present, represents the temperature measured from the external temperature probe.  This is an [`int16`](#int16) representing the temperature (divide by 256 to get degrees C).

### Soil probe (field 6)

Field 6, if present, has two readings from an external temperature/humidity probe.

- The first two bytes are a [`int16`](#int16) representing the temperature (divide by 256 to get degrees C).

- The last byte is a [`uint8`](#uint8) representing the relative humidity (divide by 2.56 to get percent).  (This field can represent humidity from 0% to 99.6%.)

## Data Formats

All multi-byte data is transmitted with the most significant byte first (big-endian format).  Comments on the individual formats follow.

### uint16

an integer from 0 to 65536.

### int16

a signed integer from -32,768 to 32,767, in two's complement form. (Thus 0..0x7FFF represent 0 to 32,767; 0x8000 to 0xFFFF represent -32,768 to -1).

### uint8

an integer from 0 to 255.

## Test Vectors

The following input data can be used to test decoders. Note that "T Dew" (the dewpoint) is computed based on temperature and RH; it's not present in the input data. MCCI's standard decoder generates this, but you may not need this.

|Input | vBat | vBus | Boot | Temp (deg C) | P (mBar) | RH % | T Dew (C) | Light |  Probe T (deg C)  | Soil T (deg C) | Soil RH % | Soil T Dew (deg C) |
|:-----|-----:|-----:|-----:|-------------:|---------:|-----:|----------:|------:|------------------:|---------------:|----------:|-------------:|
|`15 01 18 00` | +1.5 | | |  |            |           |      |     |       | |
|`15 01 F8 00` | -0.5 | | |  |            |           |      |     |       | |
|`15 05 F8 00 42` | -0.5 |  | 66 |            |           |      |     |       | |
|`15 0D F8 00 42 17 80 59 35 80` | -0.5 |  |  66 | 23.5 | 913.48 | 50 | 12.479409448936956 |  |  |  |  |  |
|`15 7D 44 60 0D 15 9D 5F CD C3 00 00 1C 11 14 46 E4` | 4.2734375 | |  13 | 21.61328125 | 981 | 76.171875 | 17.236466758309017 | 0 | 28.06640625 | 20.2734375 | 89.0625 | 18.411840342527178
|`15 7F 43 72 44 60 07 17 A4 5F CB A7 01 DB 1C 01 16 AF C3` | 4.21533203125 | 4.2734375 | 7 | 23.640625 | 980.92 | 65.234375 | 16.732001483771757 | 475 | 28.00390625 | 22.68359375 | 76.171875 | 18.271601276518467

## Node-RED Decoding Script

A Node-RED script to decode this data is part of this repository. You can download the latest version from github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/master/extra/catena-message-0x11-0x15-decoder-node-red.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-0x11-0x15-decoder-node-red.js

The MCCI decoders add dewpoint where needed. For historical reasons, the temperature probe data is labled "tWater".

## The Things Network Console decoding script

The repository contains a generic script that decodes all formats, including format 0x15, for [The Things Network console](https://console.thethingsnetwork.org).

You can get the latest version on github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/master/extra/catena-message-generic-decoder-ttn.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-generic-decoder-ttn.js

The MCCI decoders add dewpoint where needed. For historical reasons, the temperature probe data is labled "tWater".
