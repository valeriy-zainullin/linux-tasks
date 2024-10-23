#!/bin/bash

/05-get-by-last_name.run
return_code="$?"

if [ "$return_code" -ne 0 ]; then
	echo "/05-get-by-last_name.run returned $return_code."
	exit 1
fi
