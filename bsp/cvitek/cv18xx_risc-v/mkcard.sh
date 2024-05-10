#!/bin/bash

# This script is used to flash the sd-card.
# Usage:
# [DEV=xxx] [UDISK=xxx] [BOARD=duo] ./mkcard.sh [-a]
#
# Customize DEV/UDISK/BOARD with your environment.
# @DEV: device name of your sdcard, such as /dev/sdb1
# @UDISK: mount point of your sdcard, such as /opt/u-disk
# @BOARD: board type, duo/duo256m, default is duo256m if not provided
# @-a: default only flash boot.sd, if "-a" is provided, both boot.sd and fip.bin are flashed

if [[ -z "${DEV}" ]]; then
	DEV=/dev/sdb1
fi

if [[ -z "${UDISK}" ]]; then
	UDISK=/home/u/ws/u-disk
fi

if [[ -z "${BOARD}" ]]; then
        BOARD=duo256m
fi

FILES=boot.sd
if [ "$1" == "-a" ]; then
	FILES="$FILES fip.bin"
fi

ROOT=`pwd`

sudo umount $UDISK
sudo mount $DEV $UDISK
if [ 0 -ne $? ]; then
	echo "Mount failed, please check and retry!"
	exit -1
fi

echo "Removing ......"
for f in $FILES; do
	sudo rm $UDISK/$f;
done
echo "Removing Done!"

echo "Copying to sd-card ......"
for f in $FILES; do
	sudo cp $ROOT/../output/milkv-$BOARD/$f $UDISK/
done
echo "Copying Done!"

sudo umount $UDISK

echo "Done!"

