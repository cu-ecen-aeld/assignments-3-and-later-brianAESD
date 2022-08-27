#!/bin/sh
# Finder script for assignment 1

set -e
set -u

if [ $# -lt 2 ]
then
	echo "Failed: Parameter(s) not specified"
	exit 1
fi	

# First argument is a path to a directory on the filesystem
Filesdir=$1
# Second argument is a text string which will be searched within these files
Searchstr=$2

if [ ! -d "${Filesdir}" ]
then

	echo "Failed: "${Filesdir}" not a directory on the filesystem"
	exit 1
fi	

X_NUMFILES=$(find ${Filesdir} -maxdepth 2 -type f | wc -l)
Y_MACTCHLINES=$(grep -r ${Searchstr} ${Filesdir} | wc -l)

echo "The number of files are ${X_NUMFILES} and the number of matching lines are ${Y_MACTCHLINES}"

