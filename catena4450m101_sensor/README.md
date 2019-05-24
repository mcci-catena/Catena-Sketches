# Catena 4450 M101 Sensor Sketch
<!-- TOC depthFrom:2 -->

- [Clone this repository into a suitable directory on your system](#clone-this-repository-into-a-suitable-directory-on-your-system)
- [Install the MCCI SAMD board support library](#install-the-mcci-samd-board-support-library)
- [Select your desired band](#select-your-desired-band)
- [Installing the required libraries](#installing-the-required-libraries)
	- [List of required libraries](#list-of-required-libraries)
- [Build and Download](#build-and-download)
- [Load the sketch into the Catena](#load-the-sketch-into-the-catena)
- [Provision your Catena 4450](#provision-your-catena-4450)
	- [Check platform provisioning](#check-platform-provisioning)
	- [Platform Provisioning](#platform-provisioning)
	- [LoRaWAN Provisioning](#lorawan-provisioning)
		- [Preparing the network for your device](#preparing-the-network-for-your-device)
		- [Preparing your device for the network](#preparing-your-device-for-the-network)
	- [Changing registration](#changing-registration)
	- [Starting Over](#starting-over)
- [Notes](#notes)
	- [Getting Started with The Things Network](#getting-started-with-the-things-network)
		- [Create an account (if necessary)](#create-an-account-if-necessary)
		- [Create an application](#create-an-application)
		- [Add your device to the application](#add-your-device-to-the-application)
	- [Data Format](#data-format)
	- [Unplugging the USB Cable while running on batteries](#unplugging-the-usb-cable-while-running-on-batteries)
	- [Deep sleep and USB](#deep-sleep-and-usb)
	- [gitboot.sh and the other sketches](#gitbootsh-and-the-other-sketches)

<!-- /TOC -->
This sketch is used for the Ithaca power project and other AC power management applications. It's also a great starting point for doing Catena 4450 work.

It is designed for use with the [Catena 4450](https://github.com/mcci-catena/HW-Designs/tree/master/kicad/Catena-4450) in conjunction with the [Adafruit Feather M0 LoRa](https://www.adafruit.com/product/3178). In order to use this code, you must do several things:

1. Clone this repository into a suitable directory on your system.
2. Install the MCCI BSP package.
3. Install the required Arduino libraries using `git`.
4. Build and download.
5. "Provision" your Catena 4450 -- this involves entering USB commands via the Arduino serial monitor to program essential identity information into the Catena 4450, so it can join the targeted network.

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
$ cd Catena-Sketches/catena4450m101_sensor

$ # confirm that you're in the right place.
$ ls
assets/  catena4450m101_sensor.ino  git-repos.dat  README.md  VERSION.txt
```

## Install the MCCI SAMD board support library

Open the Arduino IDE. Go to `File>Preferences>Settings`. Add `https://github.com/mcci-catena/arduino-boards/raw/master/BoardManagerFiles/package_mcci_index.json` to the list in `Additional Boards Manager URLs`.

If you already have entries in that list, use a comma (`,`) to separate the entry you're adding from the entries that are already there.

Next, open the board manager. `Tools>Board:...`, and get up to the top of the menu that pops out -- it will give you a list of boards. Search for `MCCI` in the search box and select `MCCI Catena SAMD Boards`. An `[Install]` button will appear to the right; click it.

Then go to `Tools>Board:...` and scroll to the bottom. You should see `Catena 4450`; select that.

## Select your desired band

When you select a board, the default LoRaWAN region is set to US-915, which is used in North America and much of South America. If you're elsewhere, you need to select your target region. You can do it in the IDE:

![Select Bandplan](./assets/menu-region.gif)

As the animation shows, use `Tools>LoRaWAN Region...` and choose the appropriate entry from the menu.

## Installing the required libraries

This sketch uses several sensor libraries.

The script [`git-boot.sh`](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh) in the top directory of this repo will get all the things you need.

It's easy to run, provided you're on Windows, macOS, or Linux, and provided you have `git` installed. We tested on Windows with git bash from https://git-scm.org, on macOS 10.11.3 with the git and bash shipped by Apple, and on Ubuntu 16.0.4 LTS (64-bit) with the built-in bash and git from `apt-get install git`.

```console
$ cd Catena4410-Sketches/catena4450m101_sensor
$ ../git-boot.sh
Cloning into 'Adafruit_FRAM_I2C'...
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
Cloning into 'Adafruit_BME280_Library'...
remote: Counting objects: 134, done.
remote: Total 134 (delta 0), reused 0 (delta 0), pack-reused 134
Receiving objects: 100% (134/134), 39.31 KiB | 0 bytes/s, done.
Resolving deltas: 100% (68/68), done.
Cloning into 'Adafruit_Sensor'...
remote: Counting objects: 98, done.
remote: Total 98 (delta 0), reused 0 (delta 0), pack-reused 98
Unpacking objects: 100% (98/98), done.
Cloning into 'RTCZero'...
remote: Counting objects: 273, done.
Receiving remote: Total 273 (delta 0), reused 0 (delta 0), pack-reused 273
Receiving objects: 100% (273/273), 46.00 KiB | 0 bytes/s, done.
Resolving deltas: 100% (131/131), done.
Cloning into 'BH1750'...
remote: Counting objects: 58, done.
remote: Total 58 (delta 0), reused 0 (delta 0), pack-reused 58
Unpacking objects: 100% (58/58), done.

==== Summary =====
No repos with errors
No repos skipped.
*** no repos were pulled ***
Repos downloaded:      Adafruit_FRAM_I2C Catena-Arduino-Platform arduino-lorawan Catena-mcciadk arduino-lmic Adafruit_BME280_Library Adafruit_Sensor RTCZero BH1750
```

It has a number of advanced options; use `../git-boot.sh -h` to get help, or look at the source code [here](https://github.com/mcci-catena/Catena-Sketches/blob/master/git-boot.sh).

**Beware of issue #18**.  If you happen to already have libraries installed with the same names as any of the libraries in `git-repos.dat`, `git-boot.sh` will silently use the versions of the library that you already have installed. (We hope to soon fix this to at least tell you that you have a problem.)

### List of required libraries

This sketch depends on the following libraries.

*  https://github.com/mcci-catena/Adafruit_FRAM_I2C
*  https://github.com/mcci-catena/Catena4410-Arduino-Library
*  https://github.com/mcci-catena/arduino-lorawan
*  https://github.com/mcci-catena/Catena-mcciadk
*  https://github.com/mcci-catena/arduino-lmic
*  https://github.com/mcci-catena/Adafruit_BME280_Library
*  https://github.com/mcci-catena/Adafruit_Sensor
*  https://github.com/mcci-catena/RTCZero
*  https://github.com/mcci-catena/BH1750

## Build and Download

Shutdown the Arduino IDE and restart it, just in case.

Ensure selected board is 'Catena 4450' (in the GUI, check that `Tools`>`Board "..."` says `"Catena 4450"`.

In the IDE, use File>Open to load the `Catena4450m101_sensor.ino` sketch. (Remember, in step 1 you cloned `Catena-Sketches` -- find that, and navigate to `{somewhere}/Catena-Sketches/catena4450_m101/`)

Follow normal Arduino IDE procedures to build the sketch: `Sketch`>`Verify/Compile`. If there are no errors, go to the next step.

## Load the sketch into the Catena

Make sure the correct port is selected in `Tools`>`Port`.

Load the sketch into the Catena using `Sketch`>`Upload` and move on to provisioning.

## Provision your Catena 4450

This can be done with any terminal emulator, but it's easiest to do it with the serial monitor built into the Arduino IDE or with the equivalent monitor that's part of the Visual Micro IDE.

### Check platform provisioning

![Newline](./assets/serial-monitor-newline.png)

At the bottom right side of the serial monitor window, set the dropdown to `Newline` and `115200 baud`.

Enter the following command, and press enter:

```
system configure platformguid
```

If the Catena is functioning at all, you'll either get an error message, or you'll get a long number like:

```
82BF2661-70CB-45AE-B620-CAF695478BC1
```

(Several numbers are possible.)

![platformguid](./assets/system-configure-platformguid.png)

![platform number](./assets/platform-number.png)

If you get an error message, please follow the **Platform Provisioning** instructions. Otherwise, skip to **LoRAWAN Provisioning**.

### Platform Provisioning

The Catena 4450 has a number of build options. We have a single firmware image to support the various options. The firmware figures out the build options by reading data stored in the FRAM, so if the factory settings are not present or have been lost, you need to do the following.

If your Catena 4450 is fresh from the factory, you will need to enter the following commands.

- <code>system configure syseui <strong><em>serialnumber</em></strong></code>

You will find the serial number on the Catena 4450 assembly. If you can't find a serial number, please contact MCCI for assistance.

Continue by entering the following commands.

- `system configure operatingflags 1`
- `system configure platformguid 82BF2661-70CB-45AE-B620-CAF695478BC1`

### LoRaWAN Provisioning

Some background: with LoRaWAN, you have to create a project on your target network, and then register your device with that project.

Somewhat confusingly, the LoRaWAN specification uses the word "application" to refer to the group of devices in a project. We will therefore follow that convention. It's likely that your network provider follows taht convention too.

We'll be setting up the device for "over the air authentication" (or OTAA).

#### Preparing the network for your device

For OTAA, we'll need to load three items into the device. (We'll use USB to load them in -- you don't have to edit any code.)  These items are:

1. *The device extended unique identifier, or "devEUI"*. This is a 8-byte number.

   For convenience, MCCI assigns a unique identifier to each Catena; you should be able to find it on a printed label on your device. It will be a number of the form "00-02-cc-01-??-??-??-??".

2. *The application extended unique identifier, or "AppEUI"*. This is also an 8-byte number.

3. *The application key, or "AppKey"*. This is a 16-byte number.

If you're using The Things Network as your network provider, see the note below: [Getting Started with The Things Network](#getting-started-with-the-things-network). This walks you through the process of creating an application and registering a device. During that process, you will input the DevEUI (we suggest using the serial number printed on the Caten  a). At the end of the process, The Things Network will supply you with the required AppEUI and Application Key.

For other networks, follow their instructions for determining the DevEUI and getting the AppEUI and AppKey.

#### Preparing your device for the network

Make sure your device is still connected to the Arduino IDE, and make sure the serial monitor is still open. (If needed, open it using Tools>Serial Monitor.)

Enter the following commands in the serial monitor, substituting your _`DevEUI`_, _`AppEUI`_, and _`AppKey`_, one at a time.

- <code>lorawan configure deveui <em>DevEUI</em></code>
- <code>lorawan configure appeui <em>AppEUI</em></code>
- <code>lorawan configure appkey <em>AppKey</em></code>
- <code>lorawan configure join 1</code>

After ea/codecommand you will see an `OK`.

![provisioned](./assets/provisioned.png)

Then reboot your Catena (using the reset button on the upper board). You may have to close and re-open the  serial monitor after resetting the Catena.

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

Then reset your Catena, and return to [Provision your Catena 4450](#provision-your-catena-4450).

## Notes

### Getting Started with The Things Network

#### Create an account (if necessary)

Go to https://console.thethingsnetwork.org and follow the instructions to create an account.

#### Create an application

After you create an account, you'll see this screen.

![Main Console Screen](./assets/console-mainscreen.png)

You have two choices:

- Applications
- Gateways

Click on Applications, and you'll see the following.

![First Application Screen](./assets/console-application-first.png)

Click on "Get started by adding one" (or "Create a new application" if you already have one).

![Application creation form](./assets/console-application-create.png)

The first entry ("Application ID") is the unique name for your application/project. This name is used in some APIs as part of URLs, so it must follow DNS rules: it may consist only of lower-case letters, digits, and the hyphen or dash (`-`) character. For example, `sensor-project-1` is valid, but `My great project!` is not.

The second entry ("Description") is the description of your project.

The third entry is not changed by you.

The fourth entry (Handler registration) selects the router for your project. If you're in US, click in the box (left of the green check-mark), and scroll down to `ttn-handler-us-west`.

Then click "Add application".

#### Add your device to the application

You should see the following screen.

![Application screen no devices](./assets/console-application-nodevs.png)

Click on "register device" (to the right of DEVICES), and you'll see the following:

![Device registration screen](./assets/console-register-device.png)

The first field ("Device ID") is again a DNS-like name. It consists of lower-case digits, numbers and dashes, and it must not begin or end with a dash.

For the second field ("Device EUI") you have two choices.

- you can use the EUI from the printed label on your Catena, or
- you can ask console to generate an EUI for you.

Ignore the final two fields, and click "Register" at the bottom.

### Data Format

Refer to the [Protocol Description](../extra/catena-message-0x14-format.md) in the `extras` directory for information on how data is encoded.

### Unplugging the USB Cable while running on batteries

The Catena 4450 comes with a rechargable LiPo battery. This allows you to unplug the USB cable after booting the Catena 4450 without causing the Catena 4450 to restart.

Unfortunately, the Arudino USB drivers for the Catena 4450 do not distinguish between cable unplug and USB suspend. Any `Serial.print()` operation referring to the USB port will hang if the cable is unplugged after being used during a boot. The easiest work-around is to reboot the Catena after unplugging the USB cable. You can avoid this by using the Arduino UI to turn off DTR before unplugging the cable... but then you must remember to turn DTR back on. This is very fragile in practice.

### Deep sleep and USB

When the Catena 4450 is in deep sleep, the USB port will not respond to cable attaches. When the 4450 wakes up, it will connect to the PC while it is doing its work, then disconnect to go back to sleep.

While disconnected, you won't be able to select the COM port for the board from the Arduino UI. And depending on the various operatingflags settings, even after reset, you may have trouble catching the board to download a sketch before it goes to sleep.

The workaround is to "double tap" the reset button. As with any Feather M0, double-pressing the RESET button will put the Feather into download mode. To confirm this, the red light will flicker rapidly. You may have to temporarily change the download port using `Tools`>`Port`, but once the port setting is correct, you should be able to download no matter what state the board was in.

### gitboot.sh and the other sketches

The sketches in other directories in this tree are for engineering use at MCCI. The `git-repos.dat` file in this directory does not necessarily install all the required libraries needed for building the other directories. However, all the libraries should be available from https://github.com/mcci-catena/; and we are working on getting `git-repos.dat` files in every sub-directory.
