# Catena4410-Sketches
This repository contains top-level Arduino sketches for the Catena 4410, a LoRaWAN remote sensor made by MCCI based on the [Adafruit LoRa Feather M0 LoRa](https://www.adafruit.com/products/3178).

![](extra/assets/MCCI-Catena-4410-1080x620.jpg)

Further documentation on how to build and deploy is forthcoming; the sensor app takes advantage of the MCCI [arduino-lorawan](https://github.com/mcci-catena/arduino-lorawan) library to cleanly separate the application from the network transport. The application also shows how to use the RTCZero library to sleep the CPU, and how to use a variety of sensors. It's also a worked example of handling OTAA provisioning without putting keys in your primary repository. 

## catena4410_sensor1
This is the application written for the Tzu Chi University / Asolar Hualian research farm project. One firmware image is used for a variety of sensors. You can configure a given sensor as a general purpose device or as a specific subset, referencing back to the Atmel SAMD21 CPU's unique identifier.

## catena4410_test3
This is the primary test app used when bringing up and provisioning Catena 4410 units for use with The Things Network.

## catena4410_test1, catena4410_test2
These are simpler test programs. They were rarely used after test3 was ready, but they may be useful for test of future Catena 441x variants
with different sensor configurations.

# Boilerplate
MCCI work is released under the MIT public license. All other work from contributors (repositories forked to the MCCI Catena [github page](https://github.com/mcci-catena/)) are licensed according to the terms of those modules.

Support inquiries may be filed at [https:://portal.mcci.com](https:://portal.mcci.com) or as tickets on [github](https://github.com/mcci-catena). We are very busy, so we can't promise to help; but we'll do our best.

Commercial support is also available; contact MCCI for more information via our [support portal](https://portal.mcci.com) or our [web site](http://www.mcci.com).

> **Thanks, Acknowledgments, Notes**
> 
> Thanks to Amy Chen of Asolar, Josh Yu, and to Tzu-Chih University for funding the Hualian Garden project.
> 
> Further thanks to [Adafruit](https://www.adafruit.com/) for the wonderful Feather M0 LoRa platform, to [The Things Network](https://www.thethingsnetwork.org) for the LoRaWAN-based infrastructure, to [The Things Network New York](https://thethings.nyc) and [TTN Ithaca](https://ttni.tech) for the inspiration and support, and to the myriad people who have contributed to the Arduino and LoRaWAN infrastructure.
> 
>  **MCCI** and **Catena** are registered trademarks of MCCI Corporation.
>  
>  This document composed with [StackEdit](https://stackedit.io/).
