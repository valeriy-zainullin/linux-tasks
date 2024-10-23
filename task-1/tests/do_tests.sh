cd /

dmesg_cur_time() {
	dmesg | tail -n1 | sed 's/^\[ *\([0-9.]\+\)\].*$/\1/'
}

print_dmesg_between() {
	dmesg | while read -r line || [ -n "$line" ]; do
		local line_time=`echo "$line" | sed 's/^\[ *\([0-9.]\+\)\].*$/\1/'`
		local in_timespan=`echo "$1 < $line_time && $line_time < $2" | bc`
		if [ "$in_timespan" -eq 1 ]; then
			echo "$line"
		fi
	done
}

for file in /*-*.sh; do
	start_time=`dmesg_cur_time`
	#sh "$file"
  output=`sh "$file" 2>&1`
  status_code="$?"
	end_time=`dmesg_cur_time`
  if [ "$status_code" -ne 0 ]; then
		echo "$output"
    echo "$file: failed."
		echo "-- dmesg output -- "
		print_dmesg_between "$start_time" "$end_time"
		echo "-- ------------ -- "
  else
    echo "$file: passed."
  fi
done

poweroff -f
