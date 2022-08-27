#!/bin/sh
# Writer script for assignment 1

set -e
set -u

if [ $# -lt 2 ]
then
	echo "Failed: Parameter(s) not specified"
	exit 1
fi	

# First argument is full path to a file (including filename) on the filesystem
Writefile=$1
# Second argument is a text string which will be written within this file
Writestr=$2

mkdir -p  "$(dirname ${Writefile})" #newFolder2/test.txt
if [ $? -ne 0 ]; then
   echo "Failed to create."
   exit 1
fi

echo "${Writestr}" > "${Writefile}"
if [ $? -ne 0 ]; then
   echo "Failed to write."
   exit 1
fi
