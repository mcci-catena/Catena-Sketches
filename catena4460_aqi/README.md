# Catena4460-aqi example

This sketch captures a simple air-quality index, based on the gas resistance returned by the BME680 that resides in the MCCI Catena 4460. It then uses LoRaWAN technology to transmit the captured data over a suitable network, such as The Things Network.

<!-- markdownlint-disable MD004 -->
<!-- markdownlint-capture -->
<!-- markdownlint-disable -->
<!-- TOC depthFrom:2 updateOnSave:true -->

- [Description](#description)
	- [Prerequisites](#prerequisites)
	- [Air Quality Index](#air-quality-index)
		- [Known Unknowns](#known-unknowns)
		- [Published Data about Air Conductivity and Pollution](#published-data-about-air-conductivity-and-pollution)
		- [Published info about BME680](#published-info-about-bme680)
	- [Data formats](#data-formats)
- [Configuration](#configuration)
	- [Unattended mode](#unattended-mode)
	- [Test mode](#test-mode)
	- [Platform GUID](#platform-guid)
- [Provision your Catena 4460](#provision-your-catena-4460)
- [Notes](#notes)
	- [Getting Started with The Things Network](#getting-started-with-the-things-network)
	- [Unplugging the USB Cable while running on batteries](#unplugging-the-usb-cable-while-running-on-batteries)
	- [Deep sleep and USB](#deep-sleep-and-usb)
- [Boilerplate and acknowledgements](#boilerplate-and-acknowledgements)
	- [Trademarks](#trademarks)

<!-- /TOC -->
<!-- markdownlint-restore -->

## Description

The application is very simple. It sets up the measurements, then periodically measures and transmits a collection of readings:

* Temperature.
* Pressure.
* Relative Humidity.
* "Air Quality Index". This is an MCCI concept, and is different than the (closed source) IAQ offered by Bosch.
* System health indications (boot count, battery voltage).

You must provision the Catena as normal, and set up your application to understand the data format.

### Prerequisites

- Make sure you're running the MCCI Board Support Package for the Catena 4460. See [mcci-catena/arduino-boards](https://github.com/mcci-catena/arduino-boards) for information on how to set up the Arduino IDE.
- Select Catena 4460 as the target board in the Arduino IDE.
- Use [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) (at the top level of this repository) to fetch the required libraries, by using the command:

   ```shell
   cd catena4460_aqi
   ../git-boot.sh -u ./git-repos.dat
   ```

   We recommend the `-u` switch because this will ensure that libraries will be updated if already present on your system. However, this may not be what you want. Use `../git-boot.sh -H` to get help on the various options.

- Make sure you have at least v0.9.0 of the `Catena-Arduino-Platform`; otherwise the sketch will not compile.

### Air Quality Index

This sketch measures gas resistance and computes an air quality index. The method chosen is not based on the proprietary Bosch method, but we chose to use a scale from 0 to 500, as they did.

The AQI is a number ranging from 0..500, based on logarithmic gas conductivity (1/r), scaled according to the possible outputs of the BME680.  The minimum possible output of the algorithm in the public code is mapped to 500; and the maximum possible output is mapped to 0, using a linear transformation of log(1/r). No claims about this index are made in terms of how this maps to pollution. However, [public data](https://forums.pimoroni.com/t/bme680-observed-gas-ohms-readings/6608) suggests the following table (descriptive tags taken from reference):

   Quality   |  R (ohms)   |   AQI
:-----------:|:-----------:|:--------:
"good"       |  >360k      | < 160
"average"    | 184k - 360k | 160 - 190
"little bad" | 93k - 180k  | 190 - 220
"bad"        | 48k - 93k   | 220 - 250
"worse"      | 24k - 48k   | 250 - 280
"very bad"   | 12.5k - 24k | 280 - 310
"can't see the exit" | < 12.5k | > 310

#### Known Unknowns

Bosch measures gas resistance, but does not publish how this gas resistance converts to ohm-meters (the reference units for resistivity); they also do not publish the *resistance* values for their reference gases. They only publish "IAQ values", but do not disclose the algorithms. This prevents direct comparison of BME680 measurements with industry standard measurement of atmospheric conductivity.

We hope to be able to research the actual response of the BME680 so that we can make better use of the published data.

#### Published Data about Air Conductivity and Pollution

We reviewed several papers, of which the following seemed the most important:

1. Pawar et al, [Effect of relative humidity and sea level pressure on electrical conductivity of air over Indian Ocean][Pawar2009], [[Pawar2009]].

2. Kamalsi et al, [The Electrical Conductivity as an Index of Air Pollution in the Atmosphere][Kamalsi2011], [[Kamalsi2011]]

3. US Environmental Protection Agency, [EPA-450/3-82-019: Measurement of Volatile Organic Compunds - Supplement 1][EPA1982], July 1982 [[EPA1982]].

4. Wang, C., Yin, L., Zhang, L., Xiang, D., & Gao, R. (2010). [Metal Oxide Gas Sensors: Sensitivity and Influencing Factors][Wang2010],  Sensors (Basel, Switzerland), 10(3), 2088–2106. [[Wang2010]].

[[Pawar2009]] was peer-reviewed, [[Kamalsi2011]] was not, but the results looked consistent. [[EPA1982]] is a good overview of laboratory measurement techniques for VOCs and is useful background.

[[Pawar2009]] shows that relative humidity inversely affects conductivity, but the remarks, references, and data show that this is non-linear, increasing sharply when the RH > 75%. In other words, if RH > 75%, we should expect to  measure a higher resistance than we would otherwise see, and therefore the AQI should be biased upwards. The current calculation doesn't include this bias. The published effect on conductivity is modest, about 5x; this translates to about 60 points on our AQI, or two levels. In other words, we might want to fit some non-linear curve to RH, so that above 75% increases AQI, and below 75% decreases, but less strongly. On the other hand, Section 4.1 concludes that the effect of RH is less at higher pollution levels. We conclude that we don't have enough information to know how to accommodate RH; rather RH and AQI should be reported together (as is done by this sketch).

They also mention that small ion mobility decreases with increases in relative humidity; they believe that this is a smaller effect in their data, but it might be

[[Pawar2009]] also shows that there is some contribution from atmospheric pressure. It is well known that higher pressure reduces small ion mobility, so it's likely that a given value of conductance indicates more small ions if pressure is higher. Again, however, we

[Pawar2009]: https://doi.org/10.1029/2007JD009716 "Effect of relative humidity and sea level pressure on electrical conductivity of air over Indian Ocean"
[Kamalsi2011]: https://mts.intechopen.com/books/advanced-air-pollution/the-electrical-conductivity-as-an-index-of-air-pollution-in-the-atmosphere "The Electrical Conductivity as an Index of Air Pollution in the Atmosphere"
[EPA1982]: https://nepis.epa.gov/Exe/ZyNET.exe/2000MHV5.txt?ZyActionD=ZyDocument&Client=EPA&Index=2011%20Thru%202015%7C1995%20Thru%201999%7C1981%20Thru%201985%7C2006%20Thru%202010%7C1991%20Thru%201994%7C1976%20Thru%201980%7C2000%20Thru%202005%7C1986%20Thru%201990%7CPrior%20to%201976%7CHardcopy%20Publications&Docs=&Query=450382019&Time=&EndTime=&SearchMethod=2&TocRestrict=n&Toc=&TocEntry=&QField=&QFieldYear=&QFieldMonth=&QFieldDay=&UseQField=&IntQFieldOp=0&ExtQFieldOp=0&XmlQuery=&File=D%3A%5CZYFILES%5CINDEX%20DATA%5C81THRU85%5CTXT%5C00000005%5C2000MHV5.txt&User=ANONYMOUS&Password=anonymous&SortMethod=h%7C-&MaximumDocuments=15&FuzzyDegree=0&ImageQuality=r85g16/r85g16/x150y150g16/i500&Display=hpfr&DefSeekPage=x&SearchBack=ZyActionL&Back=ZyActionS&BackDesc=Results%20page&MaximumPages=1&ZyEntry=1&SeekPage=x "Measurement of Volatile Organic Compunds - Supplement 1, search link for retrieval"
[Wang2010]: http://doi.org/10.3390/s100302088 "Metal Oxide Gas Sensors: Sensitivity and Influencing Factors"

#### Published info about BME680

[uRADMonitor](https://www.uradmonitor.com/metal-oxide-voc-sensors/) has some interesting data about the BME680's response to humidity.

They publish the following chart showing the sensitivity of the MP503, apparently.

![Temperature and humidity sensitivity of MP503](https://www.uradmonitor.com/wordpress/wp-content/uploads/2018/03/MP503_temperature_humidity_characteristics-copy.jpg)

Reverse engineering the chart shows that the RH effect is consistent over temperature; so the RH can be compensated first, followed by compensating for temperature. The temperature compensation at 30% RH has a very good linear fit (R^2 = 0.9916), suggesting that the temperature compensation from this data should be (Rs/R0)(t) = -.0138*T + 1.5541.

### Data formats

It transmits a reading using `Catena_TxBuffer.h` format 0x17. This is similar to format 0x14, except that bit 0x20 of the flag byte indicates that a two-byte `uflt16` number is present representing the AQI. The AQI is a number ranging from 0..500, based on logarithmic gas conductivity (1/r), scaled according to the possible outputs of the BME680. It is divided by 512 prior to transmission in order to get into the [0,1) range limitation of the `uflt16` representation. So in the Javascript decoder, we multiply by 512 to recover the original data.

Refer to [Understanding Catena Data Format 0x14](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/catena-message-0x14-format.md) for background information.

## Configuration

Most configuration is done via USB.

### Unattended mode

The Catena library defines operating flags bit 0 (mask 0x00000001) as "unattended mode". If set, the device is supposed to use low-power sleep; if clear, the device is presumably connected to a USB host, and so doesn't go to low-power sleep (so that the USB interface continues to work).

**Note**: early versions of this sketch omitted this check. Verify in the code that your

To enable unattended mode during configuration:

```console
system configure operatingflags 1
```

To disable:

```console
system configure operatingflags 0
```

### Test mode

This sketch honors the manufacturing-test bit of operating flags (bit 1, mask 0x00000002). If bit 1 is set, it will continually measure (and print results). This will interfere with LoRa operations in test mode.

To enable manufacturing test mode during configuration

```console
system configure operatingflags 2
```

To disable:

```console
system configure operatingflags 0
```

### Platform GUID

Please set the platform GUID to `3037D9BE-8EBE-4AE7-970E-91915A2484F8` during configuration:

```console
system configure platformguid 3037D9BE-8EBE-4AE7-970E-91915A2484F8
```

## Provision your Catena 4460

In order to provision the Catena, refer the following document: [How-To-Provision-Your-Catena-Device](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/How-To-Provision-Your-Catena-Device.md).

## Notes

### Getting Started with The Things Network

These notes are in a separate file in this repository, [Getting Started with The Things Network](../extra/Getting-Started-with-The-Things-Network.md).

### Unplugging the USB Cable while running on batteries

The Catena 4460 comes with a rechargable LiPo battery. This allows you to unplug the USB cable after booting the Catena 4460 without causing the Catena 4460 to restart.

Unfortunately, the Arudino USB drivers for the Catena 4460 do not distinguish between cable unplug and USB suspend. Any `Serial.print()` operation referring to the USB port will hang if the cable is unplugged after being used during a boot. The easiest work-around is to reboot the Catena after unplugging the USB cable. You can avoid this by using the Arduino UI to turn off DTR before unplugging the cable... but then you must remember to turn DTR back on. This is very fragile in practice.

### Deep sleep and USB

When the Catena 4460 is in deep sleep, the USB port will not respond to cable attaches. When the 4460 wakes up, it will connect to the PC while it is doing its work, then disconnect to go back to sleep.

While disconnected, you won't be able to select the COM port for the board from the Arduino UI. And depending on the various operatingflags settings, even after reset, you may have trouble catching the board to download a sketch before it goes to sleep.

The workaround is to "double tap" the reset button. As with any Feather M0, double-pressing the RESET button will put the Feather into download mode. To confirm this, the red light will flicker rapidly. You may have to temporarily change the download port using `Tools`>`Port`, but once the port setting is correct, you should be able to download no matter what state the board was in.

## Boilerplate and acknowledgements

### Trademarks

- MCCI and MCCI Catena are registered trademarks of MCCI Corporation.
- LoRa is a registered trademark of Semtech Corporation.
- LoRaWAN is a registered trademark of the LoRa Alliance
- All other trademarks are properties of their respective owners.
