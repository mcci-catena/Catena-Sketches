# Catena 461x Hardware Test Sketch
<!-- TOC depthFrom:2 updateOnSave:true -->

- [Introduction](#introduction)
- [Getting Started](#getting-started)
	- [Clone this repository into a suitable directory on your system](#clone-this-repository-into-a-suitable-directory-on-your-system)
	- [Install the MCCI STM32 board support library](#install-the-mcci-stm32-board-support-library)
	- [Select your desired band](#select-your-desired-band)
	- [Installing the required libraries](#installing-the-required-libraries)
		- [List of required libraries](#list-of-required-libraries)
	- [Build and Download](#build-and-download)
	- [Load the sketch into the Catena](#load-the-sketch-into-the-catena)
- [Run test first time](#run-test-first-time)
- [Changing registration](#changing-registration)
	- [Starting Over](#starting-over)
- [Notes](#notes)
	- [Setting up DFU on a Linux or Windows PC](#setting-up-dfu-on-a-linux-or-windows-pc)
	- [LoRaWAN Data Format](#lorawan-data-format)

<!-- /TOC -->
## Introduction

This sketch is used for hardware manufacturing test of MCCI Catena&reg; 4610, 4612 and similar boards. It uses [ArduinoUnit](https://github.com/mcci-catena/arduinounit) along with the capabilities of the [Catena Arduino Platform](https://github.com/mcci-catena/Catena-Arduino-Platform) to interactively test and incrementally bring up and provision a unit under test.

Documents on the MCCI Catena 4610 family are at https://github.com/mcci-catena/HW-Designs/. 

## Getting Started

In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI Arduino board support package (BSP).
3. Install the required Arduino libraries using `git`.
4. Build the sketch and download to the unit under test.

After you have loaded the firmware, you have to run the test and check the results.

This sketch uses the Catea-Arduino-Platform library to store critical information on the integrated FRAM. There are several kinds of information. Some things only have to be programmed once in the life of the board; other things must be programmed whenever you change network connections. Entering this information this involves entering USB commands via the Arduino serial monitor.

- We call information about the 4618 that (theoretically) never changes "identity".
- We call information about the LoRaWAN "provisioning".

### Clone this repository into a suitable directory on your system

This is best done from a command line. You can use a number of techniques, but since you'll need a working git shell, we recommend using the command line.

On Windows, we strongly recommend use of "git bash", available from [git-scm.org](https://git-scm.com/download/win). Then use the "git bash" command line system that's installed by the download.

At the end of this process, you'll have a directory called `{somewhere}/Catena-Sketches`. You get to choose `{somewhere}`. Everyone has their own convention; the author typically has a directory in his home directory called `sandbox`, and then puts projects there.

Once you have a suitable command line open, you can enter the following commands. In the following, change `{somewhere}` to the directory path where you want to put `Catena-Sketches`.

```console
$ cd {somewhere}
$ git clone https://github.com/mcci-catena/Catena-Sketches
Cloning into 'Catena-Sketches'...
remote: Counting objects: 729, done.
remote: Compressing objects: 100% (17/17), done.
remote: Total 729 (delta 15), reused 20 (delta 9), pack-reused 703
Receiving objects: 100% (729/729), 714.26 KiB | 1.25 MiB/s, done.
Resolving deltas: 100% (396/396), done.

$ # get to the right subdirectory
$ cd Catena-Sketches/catena461x_test

$ # confirm that you're in the right place.
$ ls
catena461x_test.ino  git-repos.dat  README.md
```

### Install the MCCI STM32 board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena STM32 Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `MCCI Catena 4610`, `MCCI Catena 4612`, etc.; select the once matching the board to be tested.

### Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](../extra/assets/select-band-plan.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

### Installing the required libraries

This sketch uses several libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from https://git-scm.org, on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena4410-Sketches/catena461x_hwtest
$ ../git-boot.sh
Cloning into 'Catena-Arduino-Platform'...
remote: Counting objects: 1201, done.
remote: Compressing objects: 100% (36/36), done.
remote: Total 1201 (delta 27), reused 24 (delta 14), pack-reused 1151
Receiving objects: 100% (1201/1201), 275.99 KiB | 0 bytes/s, done.
Resolving deltas: 100% (900/900), done.
...

==== Summary =====
No repos with errors
No repos skipped.
*** no repos were pulled ***
Repos downloaded:      Catena-Arduino-Platform arduino-lorawan Catena-mcciadk arduino-lmic MCCI_FRAM_I2C MCCI-Catena-HS300x MCCI-Catena-HS7000x Adafruit_BME280_Library Adafruit_Sensor arduinounit
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

**Beware of issue #18**.  If you happen to already have libraries installed with the same names as any of the libraries in `git-repos.dat`, `git-boot.sh` will silently use the versions of the library that you already have installed. (We hope to soon fix this to at least tell you that you have a problem.)

#### List of required libraries

This sketch depends on the following libraries.

*  https://github.com/mcci-catena/Catena-Arduino-Platform
*  https://github.com/mcci-catena/arduino-lorawan
*  https://github.com/mcci-catena/Catena-mcciadk
*  https://github.com/mcci-catena/arduino-lmic
*  https://github.com/mcci-catena/MCCI_FRAM_I2C
*  https://github.com/mcci-catena/MCCI-Catena-HS300x
*  https://github.com/mcci-catena/MCCI-Catena-SHT3x
*  https://github.com/mcci-catena/MCCI-Catena-HS300x
*  https://github.com/mcci-catena/Adafruit_BME280_Library
*  https://github.com/mcci-catena/Adafruit_Sensor
*  https://github.com/mcci-catena/arduinounit

### Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is correct (in the GUI, check that `Tools`>`Board "..."` says `"MCCI Catena 4610"`, etc.

In the IDE, use File>Open to load the `catena461x_hwtest.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena461x_hwtest/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

### Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Run test first time

Launch the serial monitor, and examine the output. You should see something like this at the end.

   ```
   Test summary: 8 passed, 11 failed, and 2 skipped, out of 21 test(s).
   ```

Some number of failures are expected during the first pass, because the device has not been provisioned.

- Enter the following comands via the serial monitor. They won't echo, probably.

   - <code>system configure platformguid <em>xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx</em></code>

     You need to use the appropriate GUID for the build and mod-number of the device. Check the README file for [Catena Arduino Platform](https://github.com/mcci-catena/Catena-Arduino-Platform#platform-guids) and the model number of your device to find the appropriate value.

   - <code>system configure syseui 00-02-cc-01-00-00-<em>xx-yy</em></code>
   
      The digits of the syseui should be taken from the serial number label on the board. **_If there is no label, stop and get one!_**

- Reboot the board. You can use the command `system reset`, or you can press the reset button.

- Close and re-open the serial monitory. You should then see something like the following:

   ```console
   ...
   Test 4_lora_00_provisioned failed.
   Test 4_lora_10_senduplink skipped.
   Test 4_lora_20_uplink_done skipped.
   Test summary: 17 passed, 2 failed, and 2 skipped, out of 21 test(s).
   ```

  Note that more tests have passed, and fewer tests are skipped.

- Go to [The Things Network Console](https://console.thethingsnetwork.org), and register the device with an appropriate test application .  The convention is to name devices <code>device-02cc010000<em>xxyy</em></code>, where xx and yy are the last 2 bytes of the EUI. If you use bulk registration, the TTN console will do this automatically.

- Enter the following commands:

   - <code>lorawan configure deveui 00-02-cc-01-00-00-<em>xx-yy</em></code>
   - <code>lorawan configure appeui <em><strong>registration_value_from_console</strong></em></code>
   - <code>lorawan configure appkey <em><strong>registration_value_from_console</strong></em></code>
   - `lorawan configure join 1`

- Switch the TTN console view of the device to `Data`, and reboot the device. (Again, the easy way to do this is to use the `system reset` commmand.) You should see something like this:

   ```
   Test 4_lora_00_provisioned passed.
   Test 4_lora_10_senduplink passed.
   EV_JOINING
   EV_TXSTART
   EV_JOINED
   NwkID:   00000013   DevAddr: 26022f07
   EV_TXSTART
   EV_TXSTART
   EV_TXCOMPLETE
   Test 4_lora_20_uplink_done passed.
   Test summary: 21 passed, 0 failed, and 0 skipped, out of 21 test(s).
   ```

- Check the SNR of the uplink messages in the Console. We want to make sure that the antenna is connected and working. If RSSI is less than -80, or SNR is less than 8, we may have a problem.

- Check the light, pressure (for 4610/4612), RH and Vbat values for sanity. The test program also checks, but only for really outragous errors.

- Declare that the device passes.

## Changing registration

Once your device has joined the network, it's somewhat painful to unjoin. In manufacturing, we do the following procedure prior to shipping, so that the device is ready to be setup for your use case.

You need to enter a number of commands:

- `lorawan configure appskey 0`
- `lorawan configure nwkskey 0`
- `lorawan configure fcntdown 0`
- `lorawan configure fcntup 0`
- `lorawan configure devaddr 0`
- `lorawan configure netid 0`
- `lorawan configure join 0`

Then reboot the device, and confirm thta it no longer joins.

### Starting Over

If all the typing in [Changing registration](#changing-registration) is too painful, or if you're in a real hurry, you can simply reset the Catena's non-volatile memory to it's initial state. The command for this is:

- `fram reset hard`

Then reset the Catena.

## Notes

### Setting up DFU on a Linux or Windows PC

Early versions of the MCCI BSP do not include an INF file (Windows) or sample rules (Linux) to teach your system what to do. The procedures posted here show how to set things up manually: https://github.com/vpowar/LoRaWAN_SensorNetworks-Catena#uploading-code.

You may also refer to the detailed procedures that are part of the Catena 4612 user manual. Please see:

* [Catena 4612 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4611_4612/234001173a_(Catena-4612-User-Manual).pdf).

### LoRaWAN Data Format

This sketch simply sends a fixed uplink message.
