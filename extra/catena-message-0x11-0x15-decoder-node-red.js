// JavaScript source code
// This Node-RED decoding function decodes the record sent by the
// Catena 4410 or 4450 sensor application, port 1, format 0x11 or 0x15.
//
// 2017-12-13 add proper header, temp decoding.
// 2017-12-15 support formats 0x11 and 0x15.

var bytes;
if ("payload_raw" in msg)
    {
    // the console already decoded this
    bytes = msg.payload_raw;    // pick up data for convenience
    // msg.payload_fields still has the decoded data
    }
else
    {
    bytes = msg.payload;        // pick up data for convenience
    }

// create an empty table to which we'll add result fields:
//
// decoded.vBat: the battery voltage (if present)
// decoded.vBus: the USB charger voltage (if provided)
// decoded.boot: the system boot counter, modulo 256
// decoded.t: temperature in degrees C
// decoded.p: station pressure in hPa (millibars). Note that this is not
//   adjusted for the height above sealevel so can't be directly compared
//   to weather.gov "barometric pressure"
// decoded.rh: relative humidity (in %)
// decoded.lux: light level, in lux
// decoded.tWater: "water" temperature from the one-wire sensor
// decoded.tSoil: "soil" temperature from the soil sensor
// decoded.rhSoil: "soil" relative humidity
var decoded = { };

// i is the current index in bytes[]. It is incremented as byte
// values are consumed.
var i = 1;

// flags is the effective bitmap.
var flags = bytes[i++];

// validate format code and adjust flags value
if (bytes[0] == 0x11)
    {
    // 0x11 is almost the same as 0x15, but no boot count bit.
    // Simply insert a zero in the value of flags and pretend
    // this was format 0x15
    flags = ((flags & ~3) << 1) | (flags & 3);

    // set up additional metadata.
    msg.local =
        {
        nodeType: "Catena 4410",
        platformType: "Feather M0 LoRa",
        radioType: "RF95",
        applicationName: "Soil/Water monitoring"
        };
    }
else if (bytes[0] == 0x15)
    {
    // native 0x15 format. Set up metadata.
    msg.local =
        {
        nodeType: "Catena 4450-M102",
        platformType: "Feather M0 LoRa",
        radioType: "RF95",
        applicationName: "Soil/Water monitoring"
        };
    }
else
    {
    // not one of ours.
    node.error("not ours! " + bytes[0].toString());
    return;
    }

// now, start decoding flags, scanning bits from right to left.
// when we see a 1, consume corresponding bytes from message.
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

if (flags & 0x20) {
  var tempRaw = (bytes[i] << 8) + bytes[i + 1];
  // onewire temperature
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
}

// update message with desired vales.
msg.payload = decoded;

return msg;
