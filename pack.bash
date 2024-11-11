#!/bin/bash

# Запаковать только нужные файлы, чтобы отослать преподавателю.

set -e

tar --exclude='*.ko' --exclude='*.o' -c *.bash root busybox-config task-* -f - | gzip > /dev/shm/linux-tasks.tar
