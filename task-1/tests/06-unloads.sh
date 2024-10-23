#!/bin/bash

set -xe

rmmod phonebook.ko
sleep 2
dmesg | grep 'phonebook is down'
