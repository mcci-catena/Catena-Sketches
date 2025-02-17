# Understanding MCCI Catena data sent on port 2 and port 4

<!-- TOC depthFrom:2 updateOnSave:true -->

- [Overall Message Format](#overall-message-format)
- [Bitmap fields and associated fields](#bitmap-fields-and-associated-fields)
    - [Battery Voltage (field 0)](#battery-voltage-field-0)
    - [System Voltage (field 1)](#system-voltage-field-1)
    - [Boot counter (field 2)](#boot-counter-field-2)
    - [Environmental Readings (field 3)](#environmental-readings-field-3)
    - [Ambient light (field 4)](#ambient-light-field-4)
    - [Light intensity](#light-intensity-field-4)
    - [Bus Voltage (field 5)](#bus-voltage-field-5)
- [Data Formats](#data-formats)
    - [uint16](#uint16)
    - [int16](#int16)
    - [uint8](#uint8)
    - [sflt24](#sflt24)
- [Node-RED Decoding Script](#node-red-decoding-script)
- [The Things Network Console decoding script](#the-things-network-console-decoding-script)

<!-- /TOC -->

## Overall Message Format

Port 2 uplink messages are used by Catena4612_simple sketches. They're designed to minimize power use and maximize battery life; so instead of using a port code plus a message discrimintor, these simply use port 2 for version 1 boards and port 4 for version 2 boards.

Each message has the following layout.

byte | description
:---:|:---
0 | bitmap encoding the fields that follow
1..n | data bytes; use bitmap to decode.

Each bit in byte 0 represents whether a corresponding field in bytes 1..n is present. If all bits are clear, then no data bytes are present. If bit 0 is set, then field 0 is present; if bit 1 is set, then field 1 is present, and so forth. If a field is omitted, all bytes for that field are omitted.

## Bitmap fields and associated fields

The bitmap byte has the following interpretation for version 1 boards (transmitting over port 2).

Bitmap bit | Length of corresponding field (bytes) | Data format |Description
:---:|:---:|:---:|:----
0 | 2 | [int16](#int16) | [Battery voltage](#battery-voltage-field-0)
1 | 2 | [int16](#int16) | [System voltage](#system-voltage-field-1)
2 | 1 | [uint8](#uint8) | [Boot counter](#boot-counter-field-2)
3 | 5 | [int16](#int16), [uint16](#uint16), [uint8](#uint8) | [Temperature, pressure, humidity](#environmental-readings-field-3)
4 | 6 | [uint16](#uint16), [uint16](#uint16), [uint16](#uint16) | [Ambient Light](#ambient-light-field-4) (IR, white, UV)
5 | 2 | [int16](#int16) | [Bus voltage](#bus-voltage-field-5)
6 | n/a | _reserved_ | Reserved for future use.
7 | n/a | _reserved_ | Reserved for future use.

The bitmap byte has the following interpretation for version 2 boards (transmitting over port 4).

Bitmap bit | Length of corresponding field (bytes) | Data format |Description
:---:|:---:|:---:|:----
0 | 2 | [int16](#int16) | [Battery voltage](#battery-voltage-field-0)
1 | 2 | [int16](#int16) | [System voltage](#sys-voltage-field-1)
2 | 1 | [uint8](#uint8) | [Boot counter](#boot-counter-field-2)
3 | 5 | [int16](#int16), [uint16](#uint16) [uint8](#uint8) | [Temperature, pressure, humidity](#environmental-readings-field-3)
4 | 3 | [sflt24](#sflt24) | [Light intensity](#light-intensity-field-4) (Lux)
5 | 2 | [int16](#int16) | [Bus voltage](#bus-voltage-field-5)
6 | n/a | _reserved_ | Reserved for future use.
7 | n/a | _reserved_ | Reserved for future use.

NOTE: `int16`, `uint16`, etc. are defined after the table.

### Battery Voltage (field 0)

Field 0, if present, carries the current battery voltage. To get the voltage, extract the int16 value, and divide by 4096.0. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

### System Voltage (field 1)

Field 1, if present, carries the current System voltage. Divide by 4096.0 to convert from counts to volts. (Thus, this field can represent values from -8.0 volts to 7.998 volts.)

_Note:_ this field is not transmitted by some versions of the sketches.

### Boot counter (field 2)

Field 2, if present, is a counter of number of recorded system reboots, modulo 256.

### Environmental Readings (field 3)

Field 3, if present, has two environmental readings as five bytes of data.

- The first two bytes are a [`int16`](#int16) representing the temperature (divide by 256 to get degrees Celsius).

- The next two bytes are a [`uint16`](#uint16) representing the pressure (divide by 25 to get millibars).

- The last byte is a [`uint8`](#uint8) representing the relative humidity (divide by 2.56 to get percent).

### Ambient light (field 4)

For version 1 boards, Field 4, if present, has three light readings as six bytes of data.

- The first two bytes are a [`uint16`](#uint16) representing the IR measurement.
- Bytes two and three are a [`uint16`](#uint16) representing the white light measurement.
- Bytes four and five are a [`uint16`](#uint16) representing the UV light measurent.

The measurements range from 0 to 65536, but must be calibrated to obtain engineering units.

### Light intensity (field 4)

For version 2 boards, Field 4, if present, has light intensity reading as three bytes of signed float [`sflt24`](#sflt24) lux data.

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

### sflt24

a signed float from 0 to 16777215.

## Node-RED Decoding Script

A Node-RED script to decode this data is part of this repository. You can download the latest version from github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/refs/heads/master/extra/catena-message-port2-port4-decoder-node-red.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-port2-port4-decoder-node-red.js

## The Things Network Console decoding script

The repository contains a generic script that decodes all formats, including port 2 and port 4, for [The Things Network console](https://console.thethingsnetwork.org).

You can get the latest version on github:

- in raw form: https://raw.githubusercontent.com/mcci-catena/Catena-Sketches/refs/heads/master/extra/catena-message-port2-port-4-decoder-ttn.js
- or view it: https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-port2-port4-decoder-ttn.js