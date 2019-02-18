// JavaScript source code
// This Node-RED decoding function decodes the record sent by the Catena 4612
// simple sensor app.
var b;

if ("payload_raw" in msg) {
    // the console already decoded this
    b = msg.payload_raw;  // pick up data for convenience
    // msg.payload_fields still has the decoded data
}
else {
    // no console debug
    b = msg.payload;  // pick up data for conveneince
}

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
// result.aqi: air quality index
var result = {};

// check the message type byte
if (msg.port != 2) {
    // not one of ours: report an error, return without a value,
    // so that Node-RED doesn't propagate the message any further.
    node.error("not ours! " + msg.port.toString());
    return;
}

// i is used as the index into the message. Start with the flag byte.
var i = 0;
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
    result.VDD = vRaw / 4096.0;
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
    // we have light irradiance info
    var irradiance = {};
    result.irradiance = irradiance;

    var lightRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    irradiance.IR = lightRaw;

    lightRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    irradiance.White = lightRaw;

    lightRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    irradiance.UV = lightRaw;
}

if (flags & 0x20) {
    var vRaw = (b[i] << 8) + b[i + 1];
    i += 2;
    if (vRaw & 0x8000)
        vRaw += -0x10000;
    result.vBus = vRaw / 4096.0;
}

// now update msg with the new payload and new .local field
// the old msg.payload is overwritten.
msg.payload = result;
msg.local =
    {
        nodeType: "Catena 4612",
        platformType: "Catena 461x",
        radioType: "Murata",
        applicationName: "Simple sensor"
    };

return msg;
