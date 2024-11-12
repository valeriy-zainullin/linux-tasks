#!/bin/busybox sh

set -e

if [ ! -f /proc/1/nr_times_scheduled ]; then
	echo "Ошибка! Файл /proc/1/nr_times_scheduled не найден."
	busybox poweroff -f
fi

format_num() {
	local last_digit=$(echo -n "$1" | busybox tail -c1)
	if [ $(echo "$last_digit <= 4" | busybox bc) -eq 1 ]; then
		echo "$1 раза"
	else
		echo "$1 раз"
	fi
}

pid=$$
nr_times_scheduled=$(set +x; busybox cat /proc/$pid/nr_times_scheduled)
echo "Этот скрипт был запланирован $(format_num $nr_times_scheduled)."
echo "Ждем 5 секунд..."
busybox sleep 5
nr_times_scheduled=$(set +x; busybox cat /proc/$pid/nr_times_scheduled)
echo "Через 5 секунд этот скрипт был запланирован $(format_num $nr_times_scheduled) (долго ждали завершения sleep, не получали кванты времени)."
busybox poweroff -f
