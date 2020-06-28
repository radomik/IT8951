#!/bin/bash

if [ $# -lt 1 ] ; then
	echo "Usage ./$0 <eink PID>"
	echo "Run also `startx` in a different shell for a Dummy Xorg driver. See `config` directory."
	exit 1
fi

. vars.sh

while [ true ] ; do 
	scrot "$IMG_PATH"
	sudo kill -s USR1 "$1"
	while [ -f "$IMG_PATH" ] ; do 
		sleep 0.05
	done
done
