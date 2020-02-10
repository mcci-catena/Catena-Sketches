# Catena 4612 Sensor Sketch

<!--
  This TOC uses the VS Code markdown TOC extension AlanWalk.markdown-toc.
  We strongly recommend updating using VS Code, the markdown-toc extension and the
  bierner.markdown-preview-github-styles extension. Note that if you are using
  VS Code 1.29 and Markdown TOC 1.5.6, https://github.com/AlanWalk/markdown-toc/issues/65
  applies -- you must change your line-ending to some non-auto value in Settings>
  Text Editor>Files.  `\n` works for me.
-->
<!-- markdownlint-disable MD033 MD004 -->
<!-- markdownlint-capture -->
<!-- markdownlint-disable -->
<!-- TOC depthFrom:2 updateOnSave:true -->

- [Introduction](#introduction)
- [Getting Started](#getting-started)
	- [Clone this repository into a suitable directory on your system](#clone-this-repository-into-a-suitable-directory-on-your-system)
	- [Install the MCCI STM32 board support library](#install-the-mcci-stm32-board-support-library)
	- [Select your desired region](#select-your-desired-region)
	- [Installing the required libraries](#installing-the-required-libraries)
		- [List of required libraries](#list-of-required-libraries)
	- [Build and Download](#build-and-download)
	- [Load the sketch into the Catena](#load-the-sketch-into-the-catena)
- [Set the identity of your Catena 461x](#set-the-identity-of-your-catena-4612)
	- [Check platform and serial number setup](#check-platform-and-serial-number-setup)
	- [Platform Provisioning](#platform-provisioning)
- [LoRaWAN Provisioning](#lorawan-provisioning)
	- [Preparing the network for your device](#preparing-the-network-for-your-device)
	- [Preparing your device for the network](#preparing-your-device-for-the-network)
	- [Changing registration](#changing-registration)
	- [Starting Over](#starting-over)
- [Notes](#notes)
	- [Setting up DFU on a Linux or Windows PC](#setting-up-dfu-on-a-linux-or-windows-pc)
	- [Data Format](#data-format)
	- [Unplugging the USB Cable while running on batteries](#unplugging-the-usb-cable-while-running-on-batteries)
	- [Deep sleep and USB](#deep-sleep-and-usb)
	- [gitboot.sh and the other sketches](#gitbootsh-and-the-other-sketches)

<!-- /TOC -->
<!-- markdownlint-restore -->
<!-- Due to a bug in Markdown TOC, the table is formatted incorrectly if tab indentation is set other than 4. Due to another bug, this comment must be *after* the TOC entry. -->

## Introduction

This sketch demonstrates the MCCI Catena&reg; 4612 (or 4610) as a remote temperature/humidity/light sensor using a LoRaWAN&reg;-technology network to transmit to a remote server.

The Catena 4612 is a single-board LoRaWAN-enabled sensor device with the following integrated sensors:

- BME280 temperature, humidity, and pressure sensor
- Silicon Labs Si1133 light sensor
- Runs from USB or a primary battery source.

The Catena 4610 is very similar to the 4612, except that it runs from USB or a *rechargeable* battery source. It includes a LiPo battery charger.

This sketch is compatible with either board. In this README, although we focus on the 4612, you can assume that the instructions also apply to the 4610 unless we specify otherwise.

Documents on the MCCI Catena 4612 are at [`github.com/mcci-catena/HW-Designs/Boards/Catena-4612`](https://github.com/mcci-catena/HW-Designs/tree/master/Boards/Catena-4612).

* [Catena 4612 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4611_4612/234001173a_(Catena-4612-User-Manual).pdf).
* [Catena 4612 Pin Mapping Diagram](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4611_4612/234001124a_(Catena-4611_4612-PinMapping).png)

Documents on the MCCI Catena 4610 are at [`github.com/mcci-catena/HW-Designs/Boards/Catena-4610`](https://github.com/mcci-catena/HW-Designs/tree/master/Boards/Catena-4610).

* [Catena 4610 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4610/234001177a_(Catena-4610-User-Manual).pdf)
* [Catena 4610 Pin Mapping Diagram](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4610/Catena-4610-Pinmapping.png)

## Getting Started

In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI Arduino board support package (BSP).
3. Install the required Arduino libraries using `git`.
4. Build the sketch and download to your Catena 4612.

After you have loaded the firmware, you have to set up the Catena 4612.

This sketch uses the Catena-Arduino-Platform library to store critical information on the integrated FRAM. There are several kinds of information. Some things only have to be programmed once in the life of the board; other things must be programmed whenever you change network connections. Entering this information this involves entering USB commands via the Arduino serial monitor.

- We call information about the 4612 that (theoretically) never changes "identity".
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
$ cd Catena-Sketches/catena4612_simple

$ # confirm that you're in the right place.
$ ls
catena4612_simple.ino  git-repos.dat  README.md
```

### Install the MCCI STM32 board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena STM32 Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `MCCI Catena 4612`; select that.

### Select your desired region

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Region](../extra/assets/select-band-plan.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

### Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from [`git-scm.org`](https://git-scm.org), on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena-Sketches/catena4612_simple
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
Repos downloaded:      Catena-Arduino-Platform arduino-lorawan Catena-mcciadk arduino-lmic MCCI_FRAM_I2C MCCI-Catena-HS300x
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

**Beware of issue #18**.  If you happen to already have libraries installed with the same names as any of the libraries in `git-repos.dat`, `git-boot.sh` will silently use the versions of the library that you already have installed. (We hope to soon fix this to at least tell you that you have a problem.)

#### List of required libraries

This sketch depends on the following libraries.

* [`github.com/mcci-catena/Catena4410-Arduino-Library`](https://github.com/mcci-catena/Catena4410-Arduino-Library)
* [`github.com/mcci-catena/arduino-lorawan`](https://github.com/mcci-catena/arduino-lorawan)
* [`github.com/mcci-catena/Catena-mcciadk`](https://github.com/mcci-catena/Catena-mcciadk)
* [`github.com/mcci-catena/arduino-lmic`](https://github.com/mcci-catena/arduino-lmic)
* [`github.com/mcci-catena/MCCI_FRAM_I2C`](https://github.com/mcci-catena/MCCI_FRAM_I2C)
* [`github.com/mcci-catena/MCCI-Catena-HS300x`](https://github.com/mcci-catena/MCCI-Catena-HS300x)

### Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'MCCI Catena 4612' (in the GUI, check that `Tools`>`Board "..."` says `"MCCI Catena 4612"`.

In the IDE, use File>Open to load the `catena4612_simple.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena4612_simple/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

### Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Set the identity of your Catena 461x

This sketch has a command interpreter connected to the system console, normally the USB serial port. You can enter commands and get status from the Catena interactively, and you can set it up for the network without modifying the C++ code of the sketch.

This can be done with any terminal emulator, but it's easiest to do it with the serial monitor built into the Arduino IDE or with the equivalent monitor that's part of the Visual Micro IDE. It can also be done using Tera Term.

We recommend setting the terminal emulator for 115200, no parity, 8 data bits, 1 stop bit. Disable local echo in the terminal emulator, because the Catena will echo your commands as you type. Select `\r` or `\n` as your line ending character.

Commmands are entered by typing them just as you would in a terminal window on Windows, macOS, or Linux. Limited editing is supported; backspace or delete will delete a character, `^R` retypes the input line, `^U` cancels. The Catena sketch gathers your input asynchronously, so output may come from sketch while you're typing. If you get confused, just press `^R` to see what the Catena thinks you've typed, or `^U` to start over.

### Check platform and serial number setup

![Newline](./assets/serial-monitor-newline.png)

At the bottom right side of the serial monitor window, set the dropdown to `Newline` and `115200 baud`.

Enter the following command, and press enter:

```console
system configure platformguid
```

If the Catena is functioning at all, you'll either get an error message, or you'll get a long number like:

```console
915decfa-d156-4d4f-bac5-70e7724726d8
```

(Several numbers are possible.)

If you get an error message, please follow the **Platform Provisioning** instructions. Otherwise, skip to **LoRAWAN Provisioning**.

### Platform Provisioning

The Catena 4610 and 4612 have number of build options. We have a single firmware image to support the various options. The firmware learns the build options using the platform GUID data stored in the FRAM, so if the factory settings are not present or have been lost, you need to do the following.

If your Catena 4610 or 4612 is fresh from the factory, you may need to enter the following commands. As of December 2019, we do this during test.

First, check that your serial number is set by connecting via USB and a terminal emulator, and typing:

- <code>system configure syseui</code>

Then press enter.

If provisioned, the Catena will reply with a 16-digit number. If not provisioned, you will have to enter the serial number.

- <code>system configure syseui <strong><em>serialnumber</em></strong></code>

You will find the serial number on the bottom of the Catena 4610 or 4612 PCB. It will be a 16-digit number of the form `00-02-cc-01-xx-xx-xx-xx`. If you can't find a serial number, please contact MCCI for assistance.

Continue by entering the following commands.

- `system configure operatingflags`

This should display a number, `0` or `1` normally. This is a bitmask. If the low order bit is clear, then the Catena will wait for a USB computer connection before starting the sketch. If set, then the Catena will proceed, discarding output that occurs when USB is not connected. A USB charger connection *does not* count as a USB computer connection. During initial test, we suggest you leave this at `0`; but when deploying a sensor for unattended operation, you'll want to set this to `1`.

The operating flags control a number of features of the sketch and of the underlying platform. Values are given in the README for [Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform).

Next, you may need to enter a platform GUID to tell the software how the hardware is provisioned. Again, this probably is already set, but if you need to, here's how to set it.

For the Catena 4612, use:

- `system configure platformguid 915decfa-d156-4d4f-bac5-70e7724726d8`

For the Catena 4610, use:

- `system configure platformguid 53ca094b-b888-465e-aa0e-e3064ec56d21`


## LoRaWAN Provisioning

Some background: with LoRaWAN, you have to create a project on your target network, and then register your device with that project.

Somewhat confusingly, the LoRaWAN specification uses the word "application" to refer to the group of devices in a project. We will therefore follow that convention. It's likely that your network provider follows that convention too.

We'll be setting up the device for "over the air authentication" (or OTAA).

Unlike some other Arduino projects, you **do not** edit code to set up your device for the network. Instead, you use the USB serial port to enter the data, and it is stored in the Catena FRAM.

### Preparing the network for your device

For OTAA, we'll need to load three items into the device. (We'll use USB to load them in -- you don't have to edit any code.)  These items are:

1. *The device extended unique identifier, or "devEUI"*. This is a 8-byte number.

   For convenience, MCCI assigns a unique identifier to each Catena; you should be able to find it on a printed label on your device. It will be a number of the form "00-02-cc-01-??-??-??-??".

2. *The application extended unique identifier, or "AppEUI"*. This is also an 8-byte number.

3. *The application key, or "AppKey"*. This is a 16-byte number.

If you're using The Things Network as your network provider, see the notes in the separate file in this repository: [Getting Started with The Things Network](../extra/Getting-Started-With-The-Things-Network.md). This walks you through the process of creating an application and registering a device. During that process, you will input the DevEUI (we suggest using the serial number printed on the Catena). At the end of the process, The Things Network will supply you with the required AppEUI and Application Key.

For other networks, follow their instructions for determining the DevEUI and getting the AppEUI and AppKey.

### Preparing your device for the network

Make sure your device is still connected to the Arduino IDE, and make sure the serial monitor is still open. (If needed, open it using Tools>Serial Monitor.)

Enter the following commands in the serial monitor, substituting your _`DevEUI`_, _`AppEUI`_, and _`AppKey`_, one at a time.

- <code>lorawan configure deveui <em>DevEUI</em></code>
- <code>lorawan configure appeui <em>AppEUI</em></code>
- <code>lorawan configure appkey <em>AppKey</em></code>
- <code>lorawan configure join 1</code>

After each command, you will see an `OK`.

<!-- ![provisioned](./assets/provisioned.png) -->

Then reboot your Catena using the command `system reset`. If you're using the Arduino environment on Windows, you will have to close and re-open the serial monitor after resetting the Catena.

You should then see a series of messages including:

```console
EV_JOINED
NetId ...
```

### Changing registration

Once your device has joined the network, it's somewhat painful to unjoin.

You need to enter a number of commands:

- `lorawan configure appskey 0`
- `lorawan configure nwkskey 0`
- `lorawan configure fcntdown 0`
- `lorawan configure fcntup 0`
- `lorawan configure devaddr 0`
- `lorawan configure netid 0`
- `lorawan configure join 0`

Then reset your device, and repeat [LoRaWAN Provisioning](#lorawan-provisioning) above.

### Starting Over

If all the typing in [Changing registration](#changing-registration) is too painful, or if you're in a real hurry, you can simply reset the Catena's non-volatile memory to it's initial state. The command for this is:

- `fram reset hard`

Then reset your Catena, and return to [Provision your Catena 4612](#provision-your-catena-4612).

## Notes

### Setting up DFU on a Linux or Windows PC

Early versions of the MCCI BSP do not include an INF file (Windows) or sample rules (Linux) to teach your system what to do. The procedures posted here show how to set things up manually: [`github.com/vpowar/LoRaWAN_SensorNetworks-Catena#uploading-code`](https://github.com/vpowar/LoRaWAN_SensorNetworks-Catena#uploading-code).

Note that the 4610, unlike the 4612, has a dedicated boot button as well as a reset button. You can use the boot button instead of a jumper to enable on-chip DFU mode. On the 4612, you must use a jumper to enable DFU mode.

You may also refer to the detailed procedures that are part of the user manual. Please see:

* [Catena 4612 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4611_4612/234001173a_(Catena-4612-User-Manual).pdf).
* [Catena 4610 User Manual](https://github.com/mcci-catena/HW-Designs/blob/master/Boards/Catena-4610/234001177a_(Catena-4610-User-Manual).pdf)

### Data Format

Refer to the [Protocol Description](../extra/catena-message-port2-format.md) in the `extras` directory for information on how data is encoded.

### Unplugging the USB Cable while running on batteries

The Catena 4610/4612 dynamically uses power from the USB cable if it's available, even if a battery is connected. This allows you to unplug the USB cable after booting the Catena 4610/4612 without causing the Catena to restart.

Unfortunately, the Arduino USB drivers for the Catena 4610/4612 do not distinguish between cable unplug and USB suspend. Any `Serial.print()` operation referring to the USB port will hang if the cable is unplugged after being used during a boot. The easiest work-around is to reboot the Catena after unplugging the USB cable. You can avoid this by using the Arduino UI to turn off DTR before unplugging the cable... but then you must remember to turn DTR back on. This is very fragile in practice.

### Deep sleep and USB

When the Catena 4612 is in deep sleep, the USB port will not respond to cable attaches. When the 4612 wakes up, it will connect to the PC while it is doing its work, then disconnect to go back to sleep.

While disconnected, you won't be able to select the COM port for the board from the Arduino UI. And depending on the various `operatingflags` settings, even after reset, you may have trouble catching the board to download a sketch before it goes to sleep.

The workaround is use DFU boot mode to download the catena-hello sketch from [Catena-Arduino-Platform](https://github.com/mcci-catena/Catena-Arduino-Platform), and use the serial port to reset any required flags so you can get control after boot.

### gitboot.sh and the other sketches

Many of the sketches in other directories in this tree are for engineering use at MCCI. The `git-repos.dat` file in this directory does not necessarily install all the required libraries needed for building the other directories. However, many sketches have a suitable `git-repos.dat`. In any case, all the libraries should be available from [`github.com/mcci-catena/`](https://github.com/mcci-catena/); and we are working on getting `git-repos.dat` files in every sub-directory.
