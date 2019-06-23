# Understanding MCCI Catena data sent on port 3

<!-- TOC depthFrom:2 updateOnSave:true -->

- [Overall Message Format](#overall-message-format)
- [Bitmap fields and associated fields](#bitmap-fields-and-associated-fields)
	- [Battery Voltage (field 0)](#battery-voltage-field-0)
	- [System Voltage (field 1)](#system-voltage-field-1)
	- [Boot counter (field 2)](#boot-counter-field-2)
	- [Environmental Readings (field 3)](#environmental-readings-field-3)
	- [Ambient light (field 4)](#ambient-light-field-4)
	- [Bus Voltage (field 5)](#bus-voltage-field-5)
- [Data Formats](#data-formats)
	- [uint16](#uint16)
	- [int16](#int16)
	- [uint8](#uint8)
- [Test Vectors](#test-vectors)
- [Node-RED Decoding Script](#node-red-decoding-script)
- [The Things Network Console decoding script](#the-things-network-console-decoding-script)

<!-- /TOC -->

## Overall Message Format

Port 3 uplink messages are used by Catena4617_simple, Catena4618_simple and related sketches. They're designed to minimize power use and maximize battery life; so instead of using a port code plus a message discrimintor, these simply use port 3.

Each message has the following layout.

byte | description
:---:|:---
0 | bitmap encoding the fields that follow
1..n | data bytes; use bitmap to decode.

Each bit in byte 0 represents whether a corresponding field in bytes 1..n is present. If all bits are clear, then no data bytes are present. If bit 0 is set, then field 0 is present; if bit 1 is set, then field 1 is present, and so forth. If a field is omitted, all bytes for that field are omitted.

## Bitmap fields and associated fields

The bitmap byte has the following interpretation. `int16`, `uint16`, etc. are defined after the table.

Bitmap bit | Length of corresponding field (bytes) | Data format |Description
:---:|:---:|:---:|:----
0 | 2 | [int16](#int16) | [Battery voltage](#battery-voltage-field-0)
1 | 2 | [int16](#int16) | [System voltage](#sys-voltage-field-1)
2 | 1 | [uint8](#uint8) | [Boot counter](#boot-counter-field-2)
3 | 5 | [int16](#int16), [uint16](#uint16) | [Temperature, humidity](environmental-readings-field-3)
4 | 2 | [uint16](#uint16), [uint16](#uint16), [uint16](#uint16) | [Ambient Light](#ambient-light-field-4) (IR, white, UV)
5 | 2 | [int16](#int16) | [Bus voltage](#bus-voltage-field-5)
6 | n/a | _reserved_ | Reserved for future use.
7 | n/a | _reserved_ | Reserved for future use.

### Battery Voltage (field 0)

Field 0, if present, carries the current battery voltage. To get the voltage, extract the int16 value, and divide by 4096.0. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

### System Voltage (field 1)

Field 1, if present, carries the current System voltage. Divide by 4096.0 to convert from counts to volts. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

_Note:_ this field is not transmitted by some versions of the sketches.

### Boot counter (field 2)

Field 2, if present, is a counter of number of recorded system reboots, modulo 256.

### Environmental Readings (field 3)

Field 3, if present, has two environmental readings as four bytes of data.

- The first two bytes are a [`int16`](#int16) representing the temperature (divide by 256 to get degrees Celsius).

- The next two bytes are a [`uint16`](#uint16) representing the relative humidity (divide by 65535 to get percent). This field can represent humidity from 0% to 100%, in steps of roughly 0.001529%.

### Ambient light (field 4)

Field 4, if present, has three light readings as six bytes of data.

- The first two bytes are a [`uint16`](#uint16) representing the IR measurement.
- Bytes two and three are a [`uint16`](#uint16) representing the white light measurement.
- Bytes four and five are a [`uint16`](#uint16) representing the UV light measurent.

The measurements range from 0 to 65536, but must be calibrated to obtain engineering units.

### Bus Voltage (field 5)

Field 5, if present, carries the current voltage from USB VBus. Divide by 4096.0 to convert from counts to volts. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

## Data Formats

All multi-byte data is transmitted with the most significant byte first (big-endian format).  Comments on the individual formats follow.

### uint16

an integer from 0 to 65536.

### int16

a signed integer from -32,768 to 32,767, in two's complement form. (Thus 0..0x7FFF represent 0 to 32,767; 0x8000 to 0xFFFF represent -32,768 to -1).

### uint8

an integer from 0 to 255.

## Test Vectors

The following input data can be used to test decoders.

|Input | vBat | VDD  | Boot | Temp (deg C) | RH % |  IR | White |  UV | VBus |
|:-----|:----:|:----:|:----:|:------------:|:----:|:---:|:-----:|:---:|:----:|
| `01 18 00`  | +1.5 | | |  |            |      |     |       |     |      |
| `01 F8 00`  | -0.5 | | |  |            |      |     |       |     |      |
| `05 F8 00 42` | -0.5 |  | 66 |            |      |     |       |
| `3D 43 A7 2B 19 8D 8E 14 00 2E 00 49 00 03 21 23` | 4.229 |     |  43  | 25.550 | 55.5 |  46 | 73 | 3 | 2.071 |

## Node-RED Decoding Script

A Node-RED script to decode this data is part of this repository. You can download the latest version from github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/master/extra/catena-message-port3-decoder-node-red.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-port3-decoder-node-red.js

## The Things Network Console decoding script

The repository contains a generic script that decodes all formats, including port 3, for [The Things Network console](https://console.thethingsnetwork.org).

You can get the latest version on github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/master/extra/catena-message-generic-decoder-ttn.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-generic-decoder-ttn.js
