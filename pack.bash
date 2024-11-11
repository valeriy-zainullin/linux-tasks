#!/bin/bash

# Запаковать только нужные файлы, чтобы отослать преподавателю.

set -e

tar -c *.bash root task-* -f - | gzip > /dev/shm/linux-tasks.tar
