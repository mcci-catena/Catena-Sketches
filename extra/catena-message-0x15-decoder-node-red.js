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
  decoded.tWater = tempRaw / 256;
}

if (flags & 0x40) {
  // temperature followed by RH
  var tempRaw = (bytes[i] << 8) + bytes[i + 1];
  i += 2;
  var tempRH = bytes[i];
  i += 1;
  decoded.tSoil = tempRaw / 256;
  decoded.rhSoil = tempRH / 256 * 100;
}
