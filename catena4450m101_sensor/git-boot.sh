#!/bin/bash
#
# Load the repositories for building this script

REPOS='
https://github.com/mcci-catena/Adafruit_FRAM_I2C.git
https://github.com/mcci-catena/Catena4410-Arduino-Library.git
https://github.com/mcci-catena/arduino-lorawan.git
https://github.com/mcci-catena/Catena-mcciadk.git
https://github.com/mcci-catena/arduino-lmic.git
https://github.com/mcci-catena/Adafruit_BME280_Library.git
https://github.com/mcci-catena/Adafruit_Sensor.git
https://github.com/mcci-catena/RTCZero.git
https://github.com/mcci-catena/BH1750.git'

function _error {
	echo "$@" 1>&2
}

cd ~/Documents/Arduino/libraries/ || {
	_error "Can't find Arduino libraries" ; exit 1
}

for r in $REPOS ; do
	rname=$(basename $r .git)
	if [ ! -d $rname ]; then
		git clone $r || _error "Can't clone $r to $rname"
	else
		echo "repo $r already exists as $rname"
	fi
done

