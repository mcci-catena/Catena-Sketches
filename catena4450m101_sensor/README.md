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

After you've done that, you may also need to do the following:

```shell
# on Windows, this takes you to the BSP; version at end might be
# different:
cd ~/AppData/Local/arduino15/packages/adafruit/hardware/samd/1.0.13/ 

# add an upstream repository reference for the MCCI patches
git remote add mcci-catena https://github.com/mcci-catena/ArduinoCore-samd.git

# update the repository with the MCCI patches.
git fetch mcci-catena --all

# get the patched (but older branch)
git checkout TMM-Sleep

# now slide any patches that were subsequent to our snapshot 
# into your current repo
git rebase master
```
The rebase should succeed without conflicts.  If it does not, follow the instructions to abort the merge; the Arduino BSP version tagged by TMM-Sleep will be functional for you. (Meanwhile, if there's a problem, please open an issue at https://github.com/mcci-catena/ArduinoCore-samd.git, and we'll fix it as soon as we can.)
