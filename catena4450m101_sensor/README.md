This sketch is used for the Ithaca power project and other AC power management applications.

It needs the following repos to build:
*  https://github.com/mcci-catena/Adafruit_FRAM_I2C
*  https://github.com/mcci-catena/Catena4410-Arduino-Library
*  https://github.com/mcci-catena/arduino-lorawan
*  https://github.com/mcci-catena/Catena-mcciadk
*  https://github.com/mcci-catena/arduino-lmic
*  https://github.com/mcci-catena/Adafruit_BME280_Library
*  https://github.com/mcci-catena/Adafruit_Sensor
*  https://github.com/mcci-catena/RTCZero
*  https://github.com/mcci-catena/BH1750

The script 'git-boot.sh' in this directory will get all the things you need.

After you've done that, with current (July 2017) BSP packages, you have one extra manual step.

To support low-power sleep, we need to add an extra API to the Arduino BSP. If you don't do that, you'll get the following erorr when you try to compile ("verify" or "verify and download") your sketch.

```
Users/example/Documents/Arduino/catena4410_sensor1/catena4410_sensor1.ino: In function 'void settleDoneCb(osjob_t*)':
catena4410_sensor1:664: error: 'adjust_millis_forward' was not declared in this scope
     adjust_millis_forward(CATCFG_T_INTERVAL  * 1000);
                                                    ^
exit status 1
'adjust_millis_forward' was not declared in this scope
```

To update your BSP, you need to enter the following commands in git bash (on Windows) or your shell (on macOS or Linux):

```shell
# Get to the right directory for updating the BSP.
# You will need to change 1.0.18 to whaever version you have.
# With git bash on Windows:
cd ~/AppData/Local/arduino15/packages/adafruit/hardware/samd/1.0.18

# With bash on macOS
cd ~/Library/Arduino15/packages/adafruit/hardware/samd/1.0.18

# With bash on Linux
# TBD

# add an upstream repository reference for the MCCI patches
git remote add mcci-catena https://github.com/mcci-catena/ArduinoCore-samd.git

# update the repository with the MCCI patches.
git fetch mcci-catena

# get the patched (but older branch)
git checkout TMM-Sleep

# now slide any patches that were subsequent to our snapshot 
# into your current repo
git rebase master
```
The rebase should succeed without conflicts.  If it does not, follow the instructions to abort the merge; the Arduino BSP version tagged by TMM-Sleep will be functional for you. (Meanwhile, if there's a problem, please open an issue at https://github.com/mcci-catena/ArduinoCore-samd.git, and we'll fix it as soon as we can.)
