# Catena 4450 Test01 Sketch 

This sketch demonstrates the MCCI Catena&reg; 4450 as a remote temperature/humidity/light sensor.

<!-- markdownlint-disable MD004 MD033 -->
<!-- markdownlint-capture -->
<!-- markdownlint-disable -->
<!-- TOC depthFrom:2 -->

- [Clone this repository into a suitable directory on your system](#clone-this-repository-into-a-suitable-directory-on-your-system)
- [Install the MCCI SAMD board support library](#install-the-mcci-samd-board-support-library)
- [Select your desired band](#select-your-desired-band)
- [Installing the required libraries](#installing-the-required-libraries)
	- [List of required libraries](#list-of-required-libraries)
- [Build and Download](#build-and-download)
- [Load the sketch into the Catena](#load-the-sketch-into-the-catena)
- [Provision your Catena 4450](#provision-your-catena-4450)
- [Boilerplate and acknowledgements](#boilerplate-and-acknowledgements)
	- [Trademarks](#trademarks)
	
<!-- /TOC -->
<!-- markdownlint-restore -->

It is designed for use with the [Catena 4450](https://github.com/mcci-catena/HW-Designs/tree/master/kicad/Catena-4450) in conjunction with the [Adafruit Feather M0 LoRa](https://www.adafruit.com/product/3178). In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI BSP package.
3. Install the required Arduino libraries using `git`.
4. Build and download.
5. "Provision" your Catena 4450 -- this involves entering USB commands via the Arduino serial monitor to program essential identity information into the Catena 4450.

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
$ cd Catena-Sketches/catena4450_test01

$ # confirm that you're in the right place.
$ ls
assets/ catena4450_test01.ino  git-repos.dat  README.md  VERSION.txt
```

## Install the MCCI SAMD board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena SAMD Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `MCCI Catena 4450`; select that.

## Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](./assets/menu-region.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

## Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from [https://git-scm.org](https://git-scm.org), on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena4410-Sketches/catena4450_test01
$ ../git-boot.sh
Cloning into 'MCCI_FRAM_I2C'...
remote: Counting objects: 96, done.
remote: Total 96 (delta 0), reused 0 (delta 0), pack-reused 96
Unpacking objects: 100% (96/96), done.
Cloning into 'Catena-Arduino-Platform'...
remote: Counting objects: 1201, done.
remote: Compressing objects: 100% (36/36), done.
remote: Total 1201 (delta 27), reused 24 (delta 14), pack-reused 1151
Receiving objects: 100% (1201/1201), 275.99 KiB | 0 bytes/s, done.
Resolving deltas: 100% (900/900), done.
Cloning into 'Catena-mcciadk'...
remote: Counting objects: 64, done.
remote: Total 64 (delta 0), reused 0 (delta 0), pack-reused 64
Unpacking objects: 100% (64/64), done.
Cloning into 'Adafruit_BME280_Library'...
remote: Counting objects: 134, done.
remote: Total 134 (delta 0), reused 0 (delta 0), pack-reused 134
Receiving objects: 100% (134/134), 39.31 KiB | 0 bytes/s, done.
Resolving deltas: 100% (68/68), done.
Cloning into 'Adafruit_Sensor'...
remote: Counting objects: 98, done.
remote: Total 98 (delta 0), reused 0 (delta 0), pack-reused 98
Unpacking objects: 100% (98/98), done.
Cloning into 'BH1750'...
remote: Counting objects: 58, done.
remote: Total 58 (delta 0), reused 0 (delta 0), pack-reused 58
Unpacking objects: 100% (58/58), done.

==== Summary =====
No repos with errors
No repos skipped.
*** no repos were pulled ***
Repos downloaded:      MCCI_FRAM_I2C Catena-Arduino-Platform Catena-mcciadk Adafruit_BME280_Library Adafruit_Sensor BH1750
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

### List of required libraries

This sketch depends on the following libraries.

* [github.com/mcci-catena/MCCI_FRAM_I2C](https://github.com/mcci-catena/MCCI_FRAM_I2C)
* [github.com/mcci-catena/Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform)
* [github.com/mcci-catena/Catena-mcciadk](https://github.com/mcci-catena/Catena-mcciadk)
* [github.com/mcci-catena/Adafruit_BME280_Library](https://github.com/mcci-catena/Adafruit_BME280_Library)
* [github.com/mcci-catena/Adafruit_Sensor](https://github.com/mcci-catena/Adafruit_Sensor)
* [github.com/mcci-catena/BH1750](https://github.com/mcci-catena/BH1750)

## Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'MCCI Catena 4450' (in the GUI, check that `Tools`>`Board "..."` says `"MCCI Catena 4450"`.

In the IDE, use File>Open to load the `catena4450_test01.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena4450_test01/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

## Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Provision your Catena 4450

In order to provision (system) the Catena, refer the following document: [How-To-Provision-Your-Catena-Device](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/How-To-Provision-Your-Catena-Device.md).

## Boilerplate and acknowledgements

### Trademarks

- MCCI and MCCI Catena are registered trademarks of MCCI Corporation.
- All other trademarks are properties of their respective owners.