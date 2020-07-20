#!/bin/bash

if [ ${EUID} -ne 0 ]
then 
	echo "You are not running this script as root!"
	exit 1
fi

VERSION=0.5

echo "==== INSTALLING UDEV RULES ===="
cp -R ./files/* /

echo "==== CONFIG DKMS ===="
mkdir "/usr/src/t150-$VERSION"
mkdir /usr/src/build

cp -R ./t150 "/usr/src/t150-$VERSION/t150"
cp -R ./thrustmaster_enable_full "/usr/src/t150-$VERSION/thrustmaster_enable_full"
cp ./Makefile "/usr/src/t150-$VERSION/"
cp ./dkms.conf "/usr/src/t150-$VERSION/"

echo "==== DKMS ===="
dkms add -m t150 -v $VERSION
dkms build -m t150 -v $VERSION
dkms install -m t150 -v $VERSION

echo "==== LOADING NEW MODULES ===="
modprobe thrustmaster_enable_full
modprobe t150
