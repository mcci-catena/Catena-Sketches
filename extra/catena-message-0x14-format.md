# Understanding Catena 4450 format 0x14

Catena 4450 format 0x14 messages are always sent on LoRaWAN port 1, and have the following layout.

byte | description
---|---
0 | Format code (always 0x14, decimal 20).
1 | bitmap encoding the fields that follow
2..n | data bytes; use bitmap to decode.

## Bitmap fields and associated fields

The bitmap byte has the following interpretation. `int16`, `uint16`, etc. are defined after the table.

bit | length of field| encoding | description
---|---|---|---
0 | 2 | int16 | battery voltage; divide by 4096 to convert from counts to volts.
1 | 2 | int16 | bus voltage; divide by 4096 to convert from counts to volts. _Note:_ this field is currently never transmitted.
2 | 1 | uint8 | counter of number of recorded system reboots, modulo 256.
3 | 5 | int16, uint16, uint8 | Temperature (divide by 256 to get degrees C); pressure (divide by 25 to get millibars); relative humidity (divide by 2.56 to get percent).
4 | 2 | uint16 | Lux, from 0 to 65536.
5 | 4 | uint16, uint16 | Watt-hour pulses since reset, modulo 65,536. First value is for power consumed from the grid, second for power sourced to the grid.
6 | 4 | uflt16, uflt16 | Instantaneous power over this measurement period. First value is for power consumed, second is for power produced.
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
---|---
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
