#!/bin/bash

VERSION=0.8a

if [ ${EUID} -ne 0 ]
then 
	echo "You are not running this script as root!"
	exit 1
fi

echo "==== REMOVING OLD VERSIONS ===="
old_vers=$(dkms status | grep t150 | awk -F "\"*, \"*" '{print $1}' | awk -F '/' '{print $2}')
IFS=$'\n'
for version in $old_vers
do
	dkms remove -m t150 -v $version --all
done
rmmod hid_t150

echo "==== CONFIG DKMS ===="
#rm -rf /usr/src/t150-*
mkdir "/usr/src/t150-$VERSION"
mkdir "/usr/src/t150-$VERSION/build"

cp -R ./hid-t150 "/usr/src/t150-$VERSION/hid-t150"
cp ./dkms_make.mak "/usr/src/t150-$VERSION/Makefile"
cp ./dkms.conf "/usr/src/t150-$VERSION/"

echo "==== DKMS ===="
dkms add -m t150 -v $VERSION
dkms build -m t150 -v $VERSION
dkms install -m t150 -v $VERSION

echo "==== INSTALLING UDEV RULES ===="
cp -vR ./files/* /
udevadm control --reload
udevadm trigger

echo "==== LOADING NEW MODULES ===="
modprobe hid-t150
