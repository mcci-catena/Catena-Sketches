#!/bin/bash

##############################################################################
#
# Module:	gitboot.sh
#
# Function:
# 	Load the repositories for building this sketch
#
# Copyright notice:
# 	This file copyright (C) 2017 by
#
#		MCCI Corporation
#		3520 Krums Corners Road
#		Ithaca, NY 14850
#
#	Distributed under license.
#
# Author:
#	Terry Moore, MCCI	April 2017
#
##############################################################################

PNAME=$(basename "$0")
OPTDEBUG=0
OPTVERBOSE=0
#### LIBRARY_ROOT_DEFAULT: the path to the Arduino libraries on your
#### system.
LIBRARY_ROOT_DEFAULT=~/Documents/Arduino/libraries

### use a long quoted string to get the repositories
### into LIBRARY_REPOS.  Multiple lines for readabilty.
LIBRARY_REPOS='
https://github.com/mcci-catena/Adafruit_FRAM_I2C.git
https://github.com/mcci-catena/Catena4410-Arduino-Library.git
https://github.com/mcci-catena/arduino-lorawan.git
https://github.com/mcci-catena/Catena-mcciadk.git
https://github.com/mcci-catena/arduino-lmic.git
https://github.com/mcci-catena/Adafruit_BME280_Library.git
https://github.com/mcci-catena/Adafruit_Sensor.git
https://github.com/mcci-catena/RTCZero.git
https://github.com/mcci-catena/BH1750.git
'

##############################################################################
# verbose output
##############################################################################

function _verbose {
	if [ "$OPTVERBOSE" -ne 0 ]; then
		echo "$PNAME:" "$@" 1>&2
	fi
}

#### _error: define a function that will echo an error message to STDERR.
#### using "$@" ensures proper handling of quoting.
function _error {
	echo "$@" 1>&2
}

#### _fatal: print an error message and then exit the script.
function _fatal {
	_error "$@" ; exit 1
}

##############################################################################
# Scan the options
##############################################################################

LIBRARY_ROOT="${LIBRARY_ROOT_DEFAULT}"
USAGE="${PNAME} -[D H l* T v]"

#OPTDEBUG and OPTVERBOS are above
OPTDRYRUN=0

NEXTBOOL=1
while getopts DHl:nTv c
do
	# postcondition: NEXTBOOL=0 iff previous option was -n
	# in all other cases, NEXTBOOL=1
	if [ $NEXTBOOL -eq -1 ]; then
		NEXTBOOL=0
	else
		NEXTBOOL=1
	fi

	case "$c" in
	D)	OPTDEBUG=$NEXTBOOL;;
	l)	LIBRARY_ROOT="$OPTARG";;
	n)	NEXTBOOL=-1;;
	T)	OPTDRYRUN=$NEXTBOOL;;
	v)	OPTVERBOSE=$NEXTBOOL;;
	H)	less 1>&2 <<.
Pull all the repos for this project from github.

Usage:
	$USAGE

Switches:
	-D		turn on debug mode; -nD is the default.
	-l {path} 	sets the target "arduino library path".
			Default is $LIBRARY_ROOT_DEFAULT.
	-T		Do a trial run (go through the motions).
	-v		turns on verbose mode; -nv is the default.
	-H		prints this help message.
.
		exit 0;;
	\?)	echo "$USAGE" 1>&2
		exit 1;;
	esac
done

#### get rid of scanned options ####
shift `expr $OPTIND - 1`

if [ $# -ne 0 ]; then
	_error "extra arguments: $@"
fi

#### make sure LIBRARY_ROOT is really a directory
if [ ! -d "$LIBRARY_ROOT" ]; then
	_fatal "LIBRARY_ROOT: Can't find Arduino libraries:" "$LIBRARY_ROOT"
fi

#### make sure we can cd to that directory
cd "$LIBRARY_ROOT" || _fatal "can't cd:" "$LIBRARY_ROOT"

#### keep track of successes in OK_REPOS, failures in NG_REPOS,
#### and skipped repos in SKIPPED_REPOS
OK_REPOS=	#empty
NG_REPOS=	#empty
SKIPPED_REPOS=	#empty

#### scan through each of the libraries. Don't quote LIBRARY_REPOS
#### because we want bash to split it into words.
for r in $LIBRARY_REPOS ; do
	# given "https://github.com/something/somerepo.git", set rname to "somerepo"
	rname=$(basename $r .git)

	#
	# if there already is a target Arduino library of that name,
	# skip the download.
	#
	if [ -d $rname ]; then
		echo "repo $r already exists as $rname"
		SKIPPED_REPOS="${SKIPPED_REPOS}${SKIPPED_REPOS:+ }$rname"
	#
	# otherwise try to clone the repo.
	#
	else
		# clone the repo, and record results.
		if [ "$OPTDRYRUN" -ne 0 ]; then
			_verbose Dry-run: skipping git clone "$r"
		elif git clone "$r" ; then
			# add to the list; ${OK_REPOS:+ }
			# inserts a space after each repo (but nothing
			# if OK_REPOS is empty)
			OK_REPOS="${OK_REPOS}${OK_REPOS:+ }$rname"
		else
			# error; print message and remember.
			_error "Can't clone $r to $rname"
			NG_REPOS="${NG_REPOS}${NG_REPOS:+ }$rname"
		fi
	fi
done

#### print final messages
echo
echo "==== Summary ====="
if [ -z "${NG_REPOS}" ]; then
	echo "No repos with errors"
else
	echo "Repos with errors:     ${NG_REPOS}"
fi
if [ -z "${SKIPPED_REPOS}" ]; then
	echo "No repos skipped."
else
	echo "Repos already present: ${SKIPPED_REPOS}"
fi
if [ -z "${OK_REPOS}" ]; then
	echo "*** no repos were downloaded. ***"
else
	echo "Repos downloaded:      ${OK_REPOS}"
fi
if [ -z "${NG_REPOS}" ]; then
	exit 1
else
	exit 0
fi
