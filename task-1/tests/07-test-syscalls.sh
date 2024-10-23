#!/bin/bash

insmod /phonebook.ko || exit 1

/07-test-syscalls.run
return_code="$?"

if [ "$return_code" -ne 0 ]; then
	echo "/07-test-syscalls.run returned $return_code."
	exit 1
fi

