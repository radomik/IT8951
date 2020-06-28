#!/bin/bash

DIR=`basename "$(pwd)"`
DIR="/home/pi/projects/${DIR}"
REMOTE=pi@192.168.100.10

echo "Target $REMOTE:$DIR"

if [ ! -f .initialized_target ] ; then
	ssh -T $REMOTE <<- EOF
		if [ ! -d "$DIR" ] ; then
			mkdir -p "$DIR"
		fi
	EOF

	touch .initialized_target
fi

scp -r * "$REMOTE:$DIR"
ssh -T $REMOTE <<- EOF
	cd "$DIR"
	pwd
	make
EOF
