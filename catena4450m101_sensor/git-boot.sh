#!/bin/bash

##############################################################################
#
# Module:	gitboot.sh
#
# Function:
# 	Install the libraries needed for building a given sketch.
#
# Copyright notice:
# 	This file copyright (C) 2017-2018 by
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
PDIR=$(dirname "$0")
OPTDEBUG=0
OPTVERBOSE=0

##############################################################################
# verbose output
##############################################################################

function _verbose {
	if [ "$OPTVERBOSE" -ne 0 ]; then
		echo "$PNAME:" "$@" 1>&2
	fi
}

function _debug {
	if [ "$OPTDEBUG" -ne 0 ]; then
		echo "$@" 1>&2
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

function _report {
	echo "$1:"
	echo "$2" | tr ' ' '\n' | column
}


##############################################################################
# LIBRARY_ROOT_DEFAULT: the path to the Arduino libraries on your
# system. If set in the environment, we use that; otherwise we set it
# according to the OS in use.
##############################################################################
UNAME=$(uname)
if [ X"$LIBRARY_ROOT_DEFAULT" != X ]; then
  if [ ! -d "$LIBRARY_ROOT_DEFAULT" ]; then
    _error "LIBRARY_ROOT_DEFAULT not a directory: ${LIBRARY_ROOT_DEFAULT}"
  elif [ ! -x "$LIBRARY_ROOT_DEFAULT" ]; then
    _error "LIBRARY_ROOT_DEFAULT not searchable: ${LIBRARY_ROOT_DEFAULT}"
  elif [ ! -w "$LIBRARY_ROOT_DEFAULT" ]; then
    _error "LIBRARY_ROOT_DEFAULT not writable: ${LIBRARY_ROOT_DEFAULT}"
  fi
elif [ "$UNAME" = "Linux" ]; then
  LIBRARY_ROOT_DEFAULT=~/Arduino/libraries
elif [ "$UNAME" = "Darwin" ]; then
  LIBRARY_ROOT_DEFAULT=~/Documents/Arduino/libraries
elif [ "${UNAME:0:5}" = "MINGW" ]; then
  LIBRARY_ROOT_DEFAULT=~/Documents/Arduino/libraries
else
  echo "Can't detect OS: set LIBRARY_ROOT_DEFAULT to path to Arduino libraries" 1>&2
  exit 1
fi

##############################################################################
# Scan the options
##############################################################################

LIBRARY_ROOT="${LIBRARY_ROOT_DEFAULT}"
USAGE="${PNAME} -[D g l* S T u v] [datafile...]; ${PNAME} -H for help"

#OPTDEBUG and OPTVERBOSE are above
OPTDRYRUN=0
OPTGITACCESS=0
OPTUPDATE=0
OPTSKIPBLOCKING=0

NEXTBOOL=1
while getopts DgHl:nSTuv c
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
	g)	OPTGITACCESS=$NEXTBOOL;;
	l)	LIBRARY_ROOT="$OPTARG";;
	n)	NEXTBOOL=-1;;
	S)	OPTSKIPBLOCKING=$NEXTBOOL;;
	T)	OPTDRYRUN=$NEXTBOOL;;
	u)	OPTUPDATE=$NEXTBOOL;;
	v)	OPTVERBOSE=$NEXTBOOL;;
	H)	less 1>&2 <<.
Pull all the repos for this project from github.

Usage:
	$USAGE

Switches:
	-n		negates the following option.
	-D		turn on debug mode; -nD is the default.
	-g		use git access method. -ng requests
			https access method. Default is -ng.
	-l {path} 	sets the target "arduino library path".
			Default is $LIBRARY_ROOT_DEFAULT.
	-S		Skip repos that already exist; -nS means
			don't run if any repo already exist.
			Only consulted if -nu.
	-T		Do a trial run (go through the motions
			but don't do anything).
	-u		Do a git pull if repo already is found.
			-nu just skips the repository if it already
			exists. -nu is the default.
	-v		turns on verbose mode; -nv is the default.
	-H		prints this help message.

Data files:
	The arguments specify repositories to be fetched, one repository
	per line.

	Repositories may be specified with access method in the form
	https://github.com/orgname/repo.git (or similar).

	Repositories may also be specified as

		location	path

	Where location and path are separated by whitespace. In tihs gase,
	the access method (git or https) is controlled by the -g flag.

	Blank lines and comments beginning with '#' are ignored.

.
		exit 0;;
	\?)	echo "$USAGE" 1>&2
		exit 1;;
	esac
done

#### get rid of scanned options ####
shift `expr $OPTIND - 1`

if [ $# -eq 0 ]; then
	if [ -f "${PWD}/git-repos.dat" ]; then
		LIBRARY_REPOS_DAT="${PWD}/git-repos.dat"
	else
		LIBRARY_REPOS_DAT="${PDIR}/git-repos.dat"
	fi
	_verbose "setting LIBRARY_REPOS_DAT to ${LIBRARY_REPOS_DAT}"
	set -- "${LIBRARY_REPOS_DAT}"
fi

##############################################################################
# load the list of repos
##############################################################################

### use a long quoted string to get the repositories
### into LIBRARY_REPOS.  Multiple lines for readabilty.
for LIBRARY_REPOS_DAT in "$@" ; do
	if [ ! -f "${LIBRARY_REPOS_DAT}" ]; then
		_fatal "can't find git-repos data file:" "${LIBRARY_REPOS_DAT}"
	fi
done

# parse the repo file, deleting comments and eliminating duplicates
# convert two-field lines to the appropriate method, using  https://$1/$2 or git@$1:$2,
# as determined by template.
LIBRARY_REPOS=$(sed -e 's/#.*$//' "$@" | LC_ALL=C sort -u |
	awk -v OPTGITACCESS="$OPTGITACCESS" '
		(NF == 0) { next; }
		(NF == 1) {
			gsub(/\.git$/, "");
			printf("%s.git\n", $1);
			}
		(NF == 2) {
			gsub(/\.git$/, "", $2);
			if (OPTGITACCESS != 0)
				printf("git@%s:%s.git\n", $1, $2);
			else
				printf("https://%s/%s.git\n", $1, $2);
			}
		' | LC_ALL=C sort -u )

_debug "LIBRARY_REPOS: ${LIBRARY_REPOS}"

#### make sure LIBRARY_ROOT is really a directory
if [ ! -d "$LIBRARY_ROOT" ]; then
	_fatal "LIBRARY_ROOT: Can't find Arduino libraries:" "$LIBRARY_ROOT"
fi

#### make sure we can cd to that directory
cd "$LIBRARY_ROOT" || _fatal "can't cd:" "$LIBRARY_ROOT"

#### keep track of successes in CLONED_REPOS, failures in NG_REPOS,
#### and skipped repos in SKIPPED_REPOS
CLONED_REPOS=	#empty
NG_REPOS=	#empty
SKIPPED_REPOS=	#empty
PULLED_REPOS=	#empty

#### if OPTUPDATE is zero, refuse to run if there are already any libraries
#### with the target names
if [ $OPTUPDATE -eq 0 -a $OPTSKIPBLOCKING -eq 0 ]; then
	BLOCKING_REPOS=
	for r in $LIBRARY_REPOS ; do

		# given "https://github.com/something/somerepo.git",
		# set rname to "somerepo"
		rname=$(basename $r .git)

		#
		# if there already is a target Arduino library of that name, add to the
		# list of blocking libraries
		#
		if [ -d $rname ]; then
			BLOCKING_REPOS="${BLOCKING_REPOS}${BLOCKING_REPOS:+ }$rname"
		fi
	done
	if [ X"$BLOCKING_REPOS" != X ]; then
		echo
		_report  "The following repos already exist in $LIBRARY_ROOT" \
			"${BLOCKING_REPOS}"
		echo
		_fatal	"Check for conficts, or use -u to update or -S to skip blocking repos"
	fi
fi

#### scan through each of the libraries. Don't quote LIBRARY_REPOS
#### because we want bash to split it into words.
for r in $LIBRARY_REPOS ; do
	# given "https://github.com/something/somerepo.git", set rname to "somerepo"
	rname=$(basename $r .git)

	#
	# if there already is a target Arduino library of that name,
	# skip the download, or pull
	#
	if [ -d $rname ]; then
		if [ $OPTUPDATE -eq 0 ]; then
			echo "repo $r already exists as $rname, and -u not specfied"
			SKIPPED_REPOS="${SKIPPED_REPOS}${SKIPPED_REPOS:+ }$rname"
		else
			if [ "$OPTDRYRUN" -ne 0 ]; then
				_verbose Dry-run: skipping git pull "$r"
			elif ( cd $rname && _verbose $rname: && git pull ; ); then
				# add to the list; ${PULLED_REPOS:+ }
				# inserts a space after each repo (but nothing
				# if PULLED_REPOS is empty)
				PULLED_REPOS="${PULLED_REPOS}${PULLED_REPOS:+ }$rname"
			else
				# error; print message and remember.
				_error "Can't pull $r to $rname"
				NG_REPOS="${NG_REPOS}${NG_REPOS:+ }$rname"
			fi
		fi
	#
	# otherwise try to clone the repo.
	#
	else
		# clone the repo, and record results.
		if [ "$OPTDRYRUN" -ne 0 ]; then
			_verbose Dry-run: skipping git clone "$r"
		elif git clone "$r" ; then
			# add to the list; ${CLONED_REPOS:+ }
			# inserts a space after each repo (but nothing
			# if CLONED_REPOS is empty)
			CLONED_REPOS="${CLONED_REPOS}${CLONED_REPOS:+ }$rname"
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
	echo "*** No repos with errors ***"
else
	_report "Repos with errors" "${NG_REPOS}"
fi
echo

if [ -z "${SKIPPED_REPOS}" ]; then
	echo "*** No existing repos skipped ***"
else
	_report "Existing repos skipped" "${SKIPPED_REPOS}"
fi
echo

if [ -z "${PULLED_REPOS}" ]; then
	echo "*** No existing repos were updated ***"
else
	_report "Existing repos updated" "${PULLED_REPOS}"
fi
echo

if [ -z "${CLONED_REPOS}" ]; then
	echo "*** No new repos were cloned ***"
else
	_report "New repos cloned" "${CLONED_REPOS}"
fi
echo

if [ -z "${NG_REPOS}" ]; then
	exit 1
else
	exit 0
fi
