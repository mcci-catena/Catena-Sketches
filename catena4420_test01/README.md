# Catena 4420 Test01 Sketch

This sketch is used for demo of 4420 work.

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
- [Provision your Catena 4420](#provision-your-catena-4420)
	- [Check platform provisioning](#check-platform-provisioning)	- [Changing registration](#changing-registration)
	- [Starting Over](#starting-over)
- [Notes](#notes)
	- [Getting Started with The Things Network](#getting-started-with-the-things-network)
- [Boilerplate and acknowledgements](#boilerplate-and-acknowledgements)
	- [Trademarks](#trademarks)

<!-- /TOC -->
<!-- markdownlint-restore -->

In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI BSP package.
3. Install the required Arduino libraries using `git`.
4. Build and download.
5. "Provision" your Catena 4420 -- this involves entering USB commands via the Arduino serial monitor to program essential identity information into the Catena 4420, so it can join the targeted network.

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
$ cd Catena-Sketches/catena4420_test01

$ # confirm that you're in the right place.
$ ls
assets/  catena4420_test01.ino  git-repos.dat  README.md  VERSION.txt
```

## Install the MCCI SAMD board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena SAMD Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `MCCI Catena 4420`; select that.

## Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](./assets/menu-region.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

## Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from [https://git-scm.org](https://git-scm.org), on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena4410-Sketches/catena4420_test01
$ ../git-boot.sh
Cloning into 'Catena-Arduino-Platform'...
remote: Enumerating objects: 3901, done.
remote: Counting objects: 100% (777/777), done.
remote: Compressing objects: 100% (376/376), done.
remote: Total 3901 (delta 530), reused 562 (delta 386), pack-reused 3124
Receiving objects: 100% (3901/3901), 1.00 MiB | 752.00 KiB/s, done.
Resolving deltas: 100% (2922/2922), done.
Cloning into 'Catena-mcciadk'...
remote: Enumerating objects: 233, done.
remote: Counting objects: 100% (10/10), done.
remote: Compressing objects: 100% (10/10), done.
Receiving objects:  83% (remote: Total 233 (delta 0), reused 1 (delta 0), pack-reused 223
Receiving objects: 100% (233/233), 59.64 KiB | 335.00 KiB/s, done.
Resolving deltas: 100% (120/120), done.
Cloning into 'MCCI_FRAM_I2C'...
remote: Enumerating objects: 109, done.
Receiving objectsremote: Total 109 (delta 0), reused 0 (delta 0), pack-reused 109
Receiving objects: 100% (109/109), 21.91 KiB | 316.00 KiB/s, done.
Resolving deltas: 100% (52/52), done.
Cloning into 'RTCZero'...
remote: Enumerating objects: 273, done.
remote: Total 273 (delta 0), reused 0 (delta 0), pack-reused 273
Receiving objects: 100% (273/273), 46.00 KiB | 263.00 KiB/s, done.
Resolving deltas: 100% (131/131), done.
Cloning into 'arduino-lmic'...
remote: Enumerating objects: 6239, done.
remote: Counting objects: 100% (377/377), done.
remote: Compressing objects: 100% (205/205), done.
remote: Total 6239 (delta 246), reused 269 (delta 169), pack-reused 5862
Receiving objects: 100% (6239/6239), 16.84 MiB | 331.00 KiB/s, done.
Resolving deltas: 100% (4113/4113), done.
Cloning into 'arduino-lorawan'...
remote: Enumerating objects: 1294, done.
remote: Counting objects: 100% (200/200), done.
remote: Compressing objects: 100% (117/117), done.
remote: Total 1294 (delta 133), reused 133 (delta 81), pack-reused 1094
Receiving objects: 100% (1294/1294), 309.06 KiB | 420.00 KiB/s, done.
Resolving deltas: 100% (872/872), done.

==== Summary =====
*** No repos with errors ***

*** No existing repos skipped ***

*** No existing repos were updated ***

New repos cloned:                                MCCI_FRAM_I2C                         Catena-Arduino-Platform                 arduino-lmic
Catena-mcciadk                          RTCZero                                 arduino-lorawan

```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

### List of required libraries

This sketch depends on the following libraries.

* [github.com/mcci-catena/MCCI_FRAM_I2C](https://github.com/mcci-catena/MCCI_FRAM_I2C)
* [github.com/mcci-catena/Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform)
* [github.com/mcci-catena/arduino-lorawan](https://github.com/mcci-catena/arduino-lorawan)
* [github.com/mcci-catena/Catena-mcciadk](https://github.com/mcci-catena/Catena-mcciadk)
* [github.com/mcci-catena/arduino-lmic](https://github.com/mcci-catena/arduino-lmic)
* [github.com/mcci-catena/RTCZero](https://github.com/mcci-catena/RTCZero)

## Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'MCCI Catena 4420' (in the GUI, check that `Tools`>`Board "..."` says `"MCCI Catena 4420"`.

In the IDE, use File>Open to load the `catena4420_test01.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena4420_test01/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

## Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Provision your Catena 4420

In order to provision the Catena, refer the following document: [How-To-Provision-Your-Catena-Device](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/How-To-Provision-Your-Catena-Device.md).

## Notes

### Getting Started with The Things Network

These notes are in a separate file in this repository, [Getting Started with The Things Network](../extra/Getting-Started-with-The-Things-Network.md).

## Boilerplate and acknowledgements

### Trademarks

- MCCI and MCCI Catena are registered trademarks of MCCI Corporation.
- LoRa is a registered trademark of Semtech Corporation.
- LoRaWAN is a registered trademark of the LoRa Alliance
- All other trademarks are properties of their respective owners.