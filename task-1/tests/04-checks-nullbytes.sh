#!/bin/bash

# set -xe
set -e

input=""

# Assuming little endian
write_int_le() {
	# xxd output big-endian (like what we use in arithmetics) order of bytes,
	#   each byte itself is encoded as it's hex value, which is the same as just
	#   write number in 16 base, because in 01020304 (16 base) we have big-endian bytes
	#   01 02 03 04 (hex byte values). The system we use is big-endian in terms of
	#   digit order. 01_16 * 2^12 + 02_16 * 2^8 + 03_16 * 2^4 + 04_16.
	#   01_16 = 0 * 2^4 + 1 * 2^4
	#   But hex representation in big endian has equation x_1 * 16^3 (2^12) + x_2 * 16^2 + x^3 * 16 + x_4.
	# 01 02 03 04 big endian byte values is 04 03 02 01 little endian byte values.
	#   It's not as easy as just string reversal (considering we don't have spaces,
	#   which are easy to remove, simply tr -d ' ')
  # https://stackoverflow.com/questions/22296839/need-a-shell-script-to-convert-big-endian-to-little-endian
	# printf outputs 16base writing of the number, and then pads it with zeros to have 8 characters.
  # local value=`printf %08X "$1" | tac -rs .. | tr -d '\n'`
	# Не работает, т.к. у tac из busybox нет опций -r и -s.
	# Ок, просто сначала разделим на строки, а потом вызовем tac (cat в обратном порядке). Он разместит
	#   строки в обратном порядке.
	local value=`printf %08X "$1" | while read -r -n2 byte; do echo "$byte"; done | tac | tr -d '\n' `
	input="$input$value"
}

write_str() {
	local value=`echo -n "$1" | xxd -p -c"$2"`
	while [ `echo -n "$value" | wc -c` -lt `expr 2 \* "$2"` ]; do
		value="$value""00"
	done
	input="$input$value"
}

write_str_n_garbage() {
	local value=`echo -n "$1" | xxd -p -c"$2"`
	while [ `echo -n "$value" | wc -c` -lt `expr 2 \* "$2"` ]; do
		value="$value""01"
	done
	input="$input$value"
}

old_count=0
test_checks_nullbytes() {
	echo -n "$input" | xxd -p -r > /dev/pbchar || true
	count=`dmesg | grep "phonebook got user_data without null bytes in strings!" | wc -l`
	if [ "$count" -le "$old_count" ]; then
		exit 1
	fi
	old_count="$count"
	input=""
}

{
	write_int_le            1
	write_str_n_garbage     "Андрей" 64
	write_str               "Александров" 64
	write_int_le            25
	write_str               "+79999999999" 16
	write_str               "andrejalexandrov@pushkapochta.ru" 64

	test_checks_nullbytes
}

{
	write_int_le            1
	write_str               "Андрей" 64
	write_str_n_garbage     "Александров" 64
	write_int_le            25
	write_str               "+79999999999" 16
	write_str               "andrejalexandrov@pushkapochta.ru" 64

	test_checks_nullbytes
}

{
	write_int_le            1
	write_str               "Андрей" 64
	write_str               "Александров" 64
	write_int_le            25
	write_str_n_garbage     "+79999999999" 16
	write_str               "andrejalexandrov@pushkapochta.ru" 64

	test_checks_nullbytes
}

{
	write_int_le            1
	write_str               "Андрей" 64
	write_str               "Александров" 64
	write_int_le            25
	write_str               "+79999999999" 16
	write_str_n_garbage     "andrejalexandrov@pushkapochta.ru" 64

	test_checks_nullbytes
}
