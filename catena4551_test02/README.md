# Catena 4551 Sensor Sketch
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
- [Set the identity of your Catena 4551](#set-the-identity-of-your-catena-4551)
	- [Check platform amd serial number setup](#check-platform-amd-serial-number-setup)
	- [Platform Provisioning](#platform-provisioning)
	- [Starting Over](#starting-over)
- [Notes](#notes)
	- [Setting up DFU on a Linux or Windows PC](#setting-up-dfu-on-a-linux-or-windows-pc)
	- [Data Format](#data-format)
	- [Unplugging the USB Cable while running on batteries](#unplugging-the-usb-cable-while-running-on-batteries)
	- [Deep sleep and USB](#deep-sleep-and-usb)
	- [gitboot.sh and the other sketches](#gitbootsh-and-the-other-sketches)

<!-- /TOC -->
## Introduction

This sketch demonstrates the MCCI Catena&reg; 4551 as a remote temperature/humidity/light/soil sensor.

The Catena 4551 is a single-board LoRaWAN-enabled sensor device with the following integrated sensors:

- BME280 temperature, humidity, and pressure sensor
- BH1750 light sensor
- SHT1x humidity/soil sensor
- Runs from USB or a primary battery source.

Documents on the MCCI Catena 4551 are at https://github.com/mcci-catena/HW-Designs/tree/master/Boards/Catena-4551.

* [Catena 4551 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4551/How-to-Get-Started-with-Catena4551-r2.pdf).
* [Catena 4551 Pin Mapping Diagram](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4551/Catena-4551-Rev-C-Pinout.png)

## Getting Started

In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI Arduino board support package (BSP).
3. Install the required Arduino libraries using `git`.
4. Build the sketch and download to your Catena 4551.

After you have loaded the firmware, you have to set up the Catena 4551.

This sketch uses the Catena-Arduino-Platform library to store critical information on the integrated FRAM. There are several kinds of information. Some things only have to be programmed once in the life of the board; other things must be programmed whenever you change network connections. Entering this information this involves entering USB commands via the Arduino serial monitor.

- We call information about the 4551 that (theoretically) never changes "identity".

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
$ cd Catena-Sketches/catena4551_test02

$ # confirm that you're in the right place.
$ ls
catena4551_test02.ino  git-repos.dat  README.md
```

### Install the MCCI STM32 board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena STM32 Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `MCCI Catena 4551`; select that.

### Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](../extra/assets/select-band-plan.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

### Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from https://git-scm.org, on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena-Sketches/catena4551_test02
$ ../git-boot.sh
Cloning into 'Catena-Arduino-Platform'...
remote: Counting objects: 1201, done.
remote: Compressing objects: 100% (36/36), done.
remote: Total 1201 (delta 27), reused 24 (delta 14), pack-reused 1151
Receiving objects: 100% (1201/1201), 275.99 KiB | 0 bytes/s, done.
Resolving deltas: 100% (900/900), done.
...

==== Summary =====
*** No repos with errors ***

*** No existing repos skipped ***

*** No existing repos were updated ***

New repos cloned:
Adafruit_BME280_Library                 Catena-mcciadk
Adafruit_FRAM_I2C                       OneWire
Arduino-Temperature-Control-Library     SHT1x
Catena-Arduino-Platform                 BH1750                                   
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

**Beware of issue #18**.  If you happen to already have libraries installed with the same names as any of the libraries in `git-repos.dat`, `git-boot.sh` will silently use the versions of the library that you already have installed. (We hope to soon fix this to at least tell you that you have a problem.)

#### List of required libraries

This sketch depends on the following libraries.

*  https://github.com/mcci-catena/Catena-Arduino-Library
*  https://github.com/mcci-catena/Catena-mcciadk
*  https://github.com/mcci-catena/SHT1x
*  https://github.com/mcci-catena/Adafruit_BME280_Library
*  https://github.com/mcci-catena/OneWire
*  https://github.com/mcci-catena/Arduino-Temperature-Control-Library

### Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'MCCI Catena 4551' (in the GUI, check that `Tools`>`Board "..."` says `"MCCI Catena 4551"`.

In the IDE, use File>Open to load the `catena4551_test01.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena4551_test02`

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

### Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Set the identity of your Catena 4551

This can be done with any terminal emulator, but it's easiest to do it with the serial monitor built into the Arduino IDE or with the equivalent monitor that's part of the Visual Micro IDE. It can also be done usign Tera Term.

### Check platform amd serial number setup

![Newline](./assets/serial-monitor-newline.png)

At the bottom right side of the serial monitor window, set the dropdown to `Newline` and `115200 baud`.

Enter the following command, and press enter:

```
system configure platformguid
```

If the Catena is functioning at all, you'll either get an error message, or you'll get a long number like:

```
1de63656-2be7-45bd-a14c-c37086b09ab1
```

(Several numbers are possible.)

If you get an error message, please follow the **Platform Provisioning** instructions.

### Platform Provisioning

The Catena 4551 has a number of build options. We have a single firmware image to support the various options. The firmware learns the build options using the platform GUID data stored in the FRAM, so if the factory settings are not present or have been lost, you need to do the following.

If your Catena 4551 is fresh from the factory, you will need to enter the following commands.

- <code>system configure syseui <strong><em>serialnumber</em></strong></code>

You will find the serial number on the bottom of the Catena 4551 PCB. It will be a 16-digit number of the form `00-02-cc-01-xx-xx-xx-xx`. If you can't find a serial number, please contact MCCI for assistance.

Continue by entering the following commands.

- `system configure operatingflags 1`

For the Catena 4551, use:

- `system configure platformguid 1de63656-2be7-45bd-a14c-c37086b09ab1`

The operating flags control a number of features of the sketch and of the underlying platform. Values are given in the README for [Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform).

### Starting Over

To change all the platform provisioning registration, you can simply reset the Catena's non-volatile memory to it's initial state. The command for this is:

- `fram reset hard`

Then reset your Catena, and return to [Provision your Catena 4551](#provision-your-catena-4551).

## Notes

### Setting up DFU on a Linux or Windows PC

Early versions of the MCCI BSP do not include an INF file (Windows) or sample rules (Linux) to teach your system what to do. The procedures posted here show how to set things up manually: https://github.com/vpowar/LoRaWAN_SensorNetworks-Catena#uploading-code.

You may also refer to the detailed procedures that are part of the user manual. Please see:

* [Catena 4551 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4551/How-to-Get-Started-with-Catena4551-r2.pdf).

### Unplugging the USB Cable while running on batteries

The Catena 4551 dynamically uses power from the USB cable if it's available, even if a battery is connected. This allows you to unplug the USB cable after booting the Catena 4551 without causing the Catena 4551 to restart.

Unfortunately, the Arudino USB drivers for the Catena 4551 do not distinguish between cable unplug and USB suspend. Any `Serial.print()` operation referring to the USB port will hang if the cable is unplugged after being used during a boot. The easiest work-around is to reboot the Catena after unplugging the USB cable. You can avoid this by using the Arduino UI to turn off DTR before unplugging the cable... but then you must remember to turn DTR back on. This is very fragile in practice.

### Deep sleep and USB

When the Catena 4551 is in deep sleep, the USB port will not respond to cable attaches. When the 4551 wakes up, it will connect to the PC while it is doing its work, then disconnect to go back to sleep.

While disconnected, you won't be able to select the COM port for the board from the Arduino UI. And depending on the various operatingflags settings, even after reset, you may have trouble catching the board to download a sketch before it goes to sleep.

The workaround is use DFU boot mode to download the catena-hello sketch from [Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform), and use the serial port to reset any required flags so you can get control after boot.

### gitboot.sh and the other sketches

Many of the sketches in other directories in this tree are for engineering use at MCCI. The `git-repos.dat` file in this directory does not necessarily install all the required libraries needed for building the other directories. However, many sketches have a suitable `git-repos.dat`. In any case, all the libraries should be available from https://github.com/mcci-catena/; and we are working on getting `git-repos.dat` files in every sub-directory.
