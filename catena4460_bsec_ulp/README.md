# catena4460_bsec_ulp

MCCI LoRaWAN test program for the BME680, based on the Bosch Sensortec bsec library `bsec_config_state_ulp_plus.ino` example.

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
- [Provision your Catena 4460](#provision-your-catena-4460)
- [Notes](#notes)
    - [Setting up DFU on a Linux or Windows PC](#setting-up-dfu-on-a-linux-or-windows-pc)
    - [Data Format](#data-format)
    - [Unplugging the USB Cable while running on batteries](#unplugging-the-usb-cable-while-running-on-batteries)
    - [Deep sleep and USB](#deep-sleep-and-usb)
    - [gitboot.sh and the other sketches](#gitbootsh-and-the-other-sketches)
- [Bugs](#bugs)

<!-- /TOC -->
<!-- markdownlint-restore -->

It is designed for use with the [Catena 4460](https://github.com/mcci-catena/HW-Designs/tree/master/Boards/Catena-4460) in conjunction with the [Adafruit Feather M0 LoRa](https://www.adafruit.com/product/3178). In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI BSP package.
3. Install the required Arduino libraries using `git`.
4. Build and download.
5. "Provision" your Catena 4460 -- this involves entering USB commands via the Arduino serial monitor to program essential identity information into the Catena 4460, so it can join the targeted network.

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
$ cd Catena-Sketches/catena4460_bsec_ulp

$ # confirm that you're in the right place.
$ ls
assets/  catena4460_bsec_ulp.ino  git-repos.dat  README.md  VERSION.txt
```

## Install the MCCI SAMD board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena SAMD Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `MCCI Catena 4460`; select that.

## Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](./assets/menu-region.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

## Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from [https://git-scm.org](https://git-scm.org), on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena-Sketches/catena4460_bsec_ulp
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
Cloning into 'arduino-lorawan'...
remote: Counting objects: 228, done.
Recremote: Total 228 (delta 0), reused 0 (delta 0), pack-reused 228
Receiving objects: 100% (228/228), 50.30 KiB | 0 bytes/s, done.
Resolving deltas: 100% (144/144), done.
Cloning into 'Catena-mcciadk'...
remote: Counting objects: 64, done.
remote: Total 64 (delta 0), reused 0 (delta 0), pack-reused 64
Unpacking objects: 100% (64/64), done.
Cloning into 'arduino-lmic'...
remote: Counting objects: 1742, done.
remote: Compressing objects: 100% (88/88), done.
remote: Total 1742 (delta 80), reused 88 (delta 45), pack-reused 1602
Receiving objects: 100% (1742/1742), 10.27 MiB | 1.36 MiB/s, done.
Resolving deltas: 100% (1024/1024), done.
Cloning into 'BH1750'...
remote: Counting objects: 58, done.
remote: Total 58 (delta 0), reused 0 (delta 0), pack-reused 58
Unpacking objects: 100% (58/58), done.

==== Summary =====
No repos with errors
No repos skipped.
*** no repos were pulled ***
Repos downloaded:      MCCI_FRAM_I2C Catena-Arduino-Platform arduino-lorawan Catena-mcciadk arduino-lmic BSEC-Arduino-library BH1750
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

### List of required libraries

This sketch depends on the following libraries.

* [github.com/mcci-catena/MCCI_FRAM_I2C](https://github.com/mcci-catena/MCCI_FRAM_I2C)
* [github.com/mcci-catena/Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform/)
* [github.com/mcci-catena/arduino-lorawan](https://github.com/mcci-catena/arduino-lorawan)
* [github.com/mcci-catena/Catena-mcciadk](https://github.com/mcci-catena/Catena-mcciadk)
* [/github.com/mcci-catena/BSEC-Arduino-library](https://github.com/mcci-catena/BSEC-Arduino-library)
* [github.com/mcci-catena/arduino-lmic](https://github.com/mcci-catena/arduino-lmic)
* [github.com/mcci-catena/BH1750](https://github.com/mcci-catena/BH1750)

## Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'MCCI Catena 4460' (in the GUI, check that `Tools`>`Board "..."` says `"MCCI Catena 4460"`.

In the IDE, use File>Open to load the `catena4460_bsec_ulp.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena4460_bsec_ulp/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

## Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Provision your Catena 4460

In order to provision the Catena, refer the following document: [How-To-Provision-Your-Catena-Device](https://github.com/mcci-catena/Catena-Sketches/blob/master/extra/How-To-Provision-Your-Catena-Device.md).

## Notes

### Data Format

Refer to the [Protocol Description](../extra/catena-message-0x17-format.md) in the `extras` directory for information on how data is encoded.

### Unplugging the USB Cable while running on batteries

The Catena 4460 dynamically uses power from the USB cable if it's available, even if a battery is connected. This allows you to unplug the USB cable after booting the Catena 4460 without causing the Catena 4460 to restart.

Unfortunately, the Arudino USB drivers for the Catena 4460 do not distinguish between cable unplug and USB suspend. Any `Serial.print()` operation referring to the USB port will hang if the cable is unplugged after being used during a boot. The easiest work-around is to reboot the Catena after unplugging the USB cable. You can avoid this by using the Arduino UI to turn off DTR before unplugging the cable... but then you must remember to turn DTR back on. This is very fragile in practice.

### Deep sleep and USB

When the Catena 4460 is in deep sleep, the USB port will not respond to cable attaches. When the 4460 wakes up, it will connect to the PC while it is doing its work, then disconnect to go back to sleep.

While disconnected, you won't be able to select the COM port for the board from the Arduino UI. And depending on the various operatingflags settings, even after reset, you may have trouble catching the board to download a sketch before it goes to sleep.

The workaround is to "double tap" the reset button. As with any Feather M0, double-pressing the RESET button will put the Feather into download mode. To confirm this, the red light will flicker rapidly. You may have to temporarily change the download port using `Tools`>`Port`, but once the port setting is correct, you should be able to download no matter what state the board was in.

### gitboot.sh and the other sketches

The sketches in other directories in this tree are for engineering use at MCCI. The `git-repos.dat` file in this directory does not necessarily install all the required libraries needed for building the other directories. However, other directories might have unique `git-repos.dat` to clone the required libraries, also, all the libraries should be available from https://github.com/mcci-catena/;

## Bugs

The Bosch documentation doesn't make it clear when to use a different calibration file with longer period. The Catena 4460 runs at 3.3V, so we definitely need the 3.3v family. We've only tested with the 3s / 4d combination.

Adafruit and MCCI use a different default address for the BME680 than Bosch does.

Because of license restrictions, there is no CI test for this sketch.
