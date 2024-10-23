#!/bin/bash

set -xe

insmod /phonebook.ko debug=1
sleep 2
dmesg | grep 'phonebook is up'
