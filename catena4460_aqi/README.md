# catena4460-aqi example

This sketch computes a simple air-quality index based on the gas resistance returned by the BME680 that resides in the Catena 4460.

It transmits a reading using `Catena_TxBuffer.h` format 0x17. This is similar to format 0x14, except that bit 0x20 of the flag byte indicates that a two-byte `uflt16` number is present representing the AQI. The AQI is a number ranging from 0..500, based on logarithmic gas conductivity (1/r), scaled according to the possible outputs of the BME680. It is divided by 512 prior to transmission in order to get into the [0,1) range limitation of the `uflt16` representation. So in the Javascript decoder, we multiply by 512 to recover the original data.

Refer to [Understanding Catena Data Format 0x14](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-0x14-format.md) for background information.

This sketch honors the manufacturing-test bit of operating flags (bit 1, mask 0x00000002). If bit 1 is set, it will continually measure (and print results). This will interfere with LoRa operations in test mode.

Please set the platform GUID to `3037D9BE-8EBE-4AE7-970E-91915A2484F8` during configuration:

```
system configure platformguid 3037D9BE-8EBE-4AE7-970E-91915A2484F8
```
