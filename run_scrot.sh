#!/bin/bash

if [ $# -lt 1 ] ; then
	echo "Usage ./$0 <eink PID>"
	echo "Run also `startx` in a different shell for a Dummy Xorg driver. See `config` directory."
	exit 1
fi

. vars.sh

while [ true ] ; do 
	scrot "$IMG_PATH"
	if [ ! -f "$IMG_PATH" ] ; then
		echo "Fail to take screenshot to $IMG_PATH"
		exit 1
	fi
	if [ -f "$IMG_PREV_PATH" ] ; then
		SUM1=`md5sum -b "$IMG_PATH" | cut -d' ' -f1`
		SUM2=`md5sum -b "$IMG_PREV_PATH" | cut -d' ' -f1`
		if [ "$SUM1" == "$SUM2" ] ; then
			sleep 0.05
			continue
		fi
	fi
	cp "$IMG_PATH" "$IMG_PREV_PATH"
	sudo kill -s USR1 "$1"
	while [ -f "$IMG_PATH" ] ; do 
		sleep 0.05
	done
done
