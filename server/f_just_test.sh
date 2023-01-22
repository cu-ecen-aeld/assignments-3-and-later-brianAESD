#!/bin/bash
# This script can be copied into your base directory for use with
# automated testing using assignment-autotest.  It automates the
# steps described in https://github.com/cu-ecen-5013/assignment-autotest/blob/master/README.md#running-tests
set -e
    echo "date: $(($(date +%s%N)/1000000))" #bwan
	echo "___......bwan: 0b aesdsocket pid: $(pgrep aesdsocket)" #bwan
	echo "___......bwan: 0b  v aesdsocket pid: $(pgrep -f "valgrind.*aesdsocket")" #bwan
#./aesdsocket
valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=definite --verbose --log-file=valgrind-out.txt ./aesdsocket
    echo "date: $(($(date +%s%N)/1000000))" #bwan
	echo "___......bwan: 0b aesdsocket pid: $(pgrep aesdsocket)" #bwan
	echo "___......bwan: 0b  v aesdsocket pid: $(pgrep -f "valgrind.*aesdsocket")" #bwan

