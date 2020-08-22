#!/bin/bash

INIT_DRIVER_REPO="https://github.com/scarburato/hid-tminit"
VERSION=0.7a

if [ ${EUID} -ne 0 ]
then 
	echo "You are not running this script as root!"
	exit 1
fi

echo "==== REMOVING OLD VERSIONS ===="

echo "==== CONFIG DKMS ===="
#rm -rf /usr/src/t150-*
mkdir "/usr/src/t150-$VERSION"
mkdir "/usr/src/t150-$VERSION/build"

cp -R ./t150 "/usr/src/t150-$VERSION/t150"
mkdir "/usr/src/t150-$VERSION/hid-tminit"
git clone $INIT_DRIVER_REPO "/usr/src/t150-$VERSION/hid-tminit"
cp ./dkms_make.mak "/usr/src/t150-$VERSION/Makefile"
cp ./dkms.conf "/usr/src/t150-$VERSION/"

echo "==== DKMS ===="
dkms add -m t150 -v $VERSION
dkms build -m t150 -v $VERSION
dkms install -m t150 -v $VERSION

echo "==== SET UP LOAD AT BOOT ===="
sed -i '/t150/d' /etc/modules
sed -i '/hid-tminit/d' /etc/modules

echo "t150" >> /etc/modules
echo "hid-tminit" >> /etc/modules

echo "==== INSTALLING UDEV RULES ===="
cp -vR ./files/* /
udevadm control --reload
udevadm trigger

echo "==== LOADING NEW MODULES ===="
modprobe hid-tminit
modprobe t150
