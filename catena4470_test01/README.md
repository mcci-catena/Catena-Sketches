# Catena 4470 Sensor Sketch
<!-- TOC depthFrom:2 -->
- [Clone this repository into a suitable directory on your system](#clone-this-repository-into-a-suitable-directory-on-your-system)
- [Install the MCCI SAMD board support library](#install-the-mcci-samd-board-support-library)
- [Select your desired band](#select-your-desired-band)
- [Installing the required libraries](#installing-the-required-libraries)
	- [List of required libraries](#list-of-required-libraries)
- [Build and Download](#build-and-download)
- [Load the sketch into the Catena](#load-the-sketch-into-the-catena)
- [Provision your Catena 4470](#provision-your-catena-4470)
- [Notes](#notes)
	- [Getting Started with The Things Network](#getting-started-with-the-things-network)
	- [Data Format](#data-format)
	- [Unplugging the USB Cable while running on batteries](#unplugging-the-usb-cable-while-running-on-batteries)
	- [Deep sleep and USB](#deep-sleep-and-usb)
	- [gitboot.sh and the other sketches](#gitbootsh-and-the-other-sketches)
<!-- /TOC -->
This sketch demonstrates Catena 4470 as a remote temperature/humidity/light sensor and modbus host using a LoRaWAN&reg;-techology network to transmit to a remote server.

It is designed for use with the [Catena 4470](https://github.com/mcci-catena/HW-Designs/tree/master/Boards/Catena-4470) in conjunction with the [Adafruit Feather M0 LoRa](https://www.adafruit.com/product/3178). In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI BSP package.
3. Install the required Arduino libraries using `git`.
4. Configure modbus device parameters(slave address, function code, start address, number of coils, baudrate) under the function `setup_modbus()`.
5. Build and download.
6. "Provision" your Catena 4470 -- this involves entering USB commands via the Arduino serial monitor to program essential identity information into the Catena 4470, so it can join the targeted network.

## Clone this repository into a suitable directory on your system

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
$ cd Catena-Sketches/catena4470_test01

$ # confirm that you're in the right place.
$ ls
catena4470_test01.ino  git-repos.dat  README.md
```

## Install the MCCI SAMD board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena SAMD Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `Catena 4470`; select that.

## Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](../extra/assets/menu-region.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

## Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from https://git-scm.org, on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena-Sketches/catena4470_test01
$ ../git-boot.sh
Cloning into 'Adafruit_BME280_Library'...
remote: Enumerating objects: 134, done.
remote: Total 134 (delta 0), reused 0 (delta 0), pack-reused 134
Receiving objects: 100% (134/134), 39.31 KiB | 1.46 MiB/s, done.
Resolving deltas: 100% (68/68), done.
Cloning into 'Adafruit_FRAM_I2C'...
remote: Enumerating objects: 96, done.
remote: Total 96 (delta 0), reused 0 (delta 0), pack-reused 96
Unpacking objects: 100% (96/96), done.
Cloning into 'Adafruit_Sensor'...
remote: Enumerating objects: 98, done.
remote: Total 98 (delta 0), reused 0 (delta 0), pack-reused 98
Unpacking objects: 100% (98/98), done.
Cloning into 'BH1750'...
remote: Enumerating objects: 77, done.
remote: Total 77 (delta 0), reused 0 (delta 0), pack-reused 77
Unpacking objects: 100% (77/77), done.
Cloning into 'Catena-Arduino-Platform'...
remote: Enumerating objects: 6, done.
remote: Counting objects: 100% (6/6), done.
remote: Compressing objects: 100% (6/6), done.
remote: Total 2494 (delta 1), reused 2 (delta 0), pack-reused 2488
Receiving objects: 100% (2494/2494), 632.57 KiB | 2.10 MiB/s, done.
Resolving deltas: 100% (1901/1901), done.
Checking out files: 100% (167/167), done.
Cloning into 'Catena-mcciadk'...
remote: Enumerating objects: 33, done.
remote: Counting objects: 100% (33/33), done.
remote: Compressing objects: 100% (23/23), done.
remote: Total 184 (delta 14), reused 19 (delta 9), pack-reused 151
Receiving objects: 100% (184/184), 49.38 KiB | 1.45 MiB/s, done.
Resolving deltas: 100% (89/89), done.
Cloning into 'Modbus-for-Arduino'...
remote: Enumerating objects: 298, done.
Receiremote: Total 298 (delta 0), reused 0 (delta 0), pack-reused 298
Receiving objects: 100% (298/298), 217.49 KiB | 2.65 MiB/s, done.
Resolving deltas: 100% (130/130), done.
Cloning into 'arduino-lmic'...
remote: Enumerating objects: 115, done.
remote: Counting objects: 100% (115/115), done.
remote: Compressing objects: 100% (49/49), done.
remote: Total 3938 (delta 69), reused 96 (delta 62), pack-reused 3823
Receiving objects: 100% (3938/3938), 11.92 MiB | 1.78 MiB/s, done.
Resolving deltas: 100% (2566/2566), done.
Checking out files: 100% (87/87), done.
Cloning into 'arduino-lorawan'...
remote: Enumerating objects: 192, done.
remote: Counting objects: 100% (192/192), done.
remote: Compressing objects: 100% (92/92), done.
remote: Total 781 (delta 120), reused 161 (delta 98), pack-reused 589
Receiving objects: 100% (781/781), 189.71 KiB | 2.34 MiB/s, done.
Resolving deltas: 100% (481/481), done.
Checking out files: 100% (44/44), done.

==== Summary =====
*** No repos with errors ***

*** No existing repos skipped ***

*** No existing repos were updated ***

New repos cloned:
Adafruit_BME280_Library BH1750                  Modbus-for-Arduino
Adafruit_FRAM_I2C       Catena-Arduino-Platform arduino-lmic
Adafruit_Sensor         Catena-mcciadk          arduino-lorawan
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

**Beware of issue #18**.  If you happen to already have libraries installed with the same names as any of the libraries in `git-repos.dat`, `git-boot.sh` will silently use the versions of the library that you already have installed. (We hope to soon fix this to at least tell you that you have a problem.)

### List of required libraries

This sketch depends on the following libraries.

*  https://github.com/mcci-catena/Adafruit_BME280_Library
*  https://github.com/mcci-catena/Adafruit_FRAM_I2C
*  https://github.com/mcci-catena/Adafruit_Sensor
*  https://github.com/mcci-catena/arduino-lmic
*  https://github.com/mcci-catena/arduino-lorawan
*  https://github.com/mcci-catena/BH1750
*  https://github.com/mcci-catena/Catena-Arduino-Platform
*  https://github.com/mcci-catena/Catena-mcciadk
*  https://github.com/mcci-catena/Modbus-for-Arduino

## Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'Catena 4470' (in the GUI, check that `Tools`>`Board "..."` says `"Catena 4470"`.

In the IDE, use File>Open to load the `Catena4470_test01.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/Catena4470_test01/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

## Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Provision your Catena 4470

In order to provision the Catena, refer the following document: [How-To-Provision-Your-Catena-Device](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/How-To-Provision-Your-Catena-Device.md).

## Notes

### Getting Started with The Things Network

These notes are in a separate file in this repository, [Getting Started with The Things Network](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/Getting-Started-With-The-Things-Network.md).

### Data Format

Refer to the [Protocol Description](../extra/catena-message-0x15-format.md) in the `extras` directory for information on how data is encoded. This data format do not include modbus decoding.

### Unplugging the USB Cable while running on batteries

The Catena 4470 comes with a rechargable LiPo battery. This allows you to unplug the USB cable after booting the Catena 4470 without causing the Catena 4470 to restart.

Unfortunately, the Arudino USB drivers for the Catena 4470 do not distinguish between cable unplug and USB suspend. Any `Serial.print()` operation referring to the USB port will hang if the cable is unplugged after being used during a boot. The easiest work-around is to reboot the Catena after unplugging the USB cable. You can avoid this by using the Arduino UI to turn off DTR before unplugging the cable... but then you must remember to turn DTR back on. This is very fragile in practice.

### Deep sleep and USB

When the Catena 4470 is in deep sleep, the USB port will not respond to cable attaches. When the 4470 wakes up, it will connect to the PC while it is doing its work, then disconnect to go back to sleep.

While disconnected, you won't be able to select the COM port for the board from the Arduino UI. And depending on the various operatingflags settings, even after reset, you may have trouble catching the board to download a sketch before it goes to sleep.

The workaround is to "double tap" the reset button. As with any Feather M0, double-pressing the RESET button will put the Feather into download mode. To confirm this, the red light will flicker rapidly. You may have to temporarily change the download port using `Tools`>`Port`, but once the port setting is correct, you should be able to download no matter what state the board was in.

### gitboot.sh and the other sketches

The sketches in other directories in this tree are for engineering use at MCCI. The `git-repos.dat` file in this directory does not necessarily install all the required libraries needed for building the other directories. However, all the libraries should be available from https://github.com/mcci-catena/;