#!/usr/bin/env python3
import usb1
from os import path

#msg = chr(0x40) + chr(0x03) + chr(0x0c) + chr(0x00)
#msg = bytearray(b'\x40\x03\x00\x00')

#msg = bytearray(b'\x40\x11\xFF\x7F')
msg = bytearray(b'\x40\x03\x01\x00')

_context = usb1.USBContext()

handle = _context.openByVendorIDAndProductID(
	0x044f, 0xb677
)

handle.setAutoDetachKernelDriver(True)
handle.claimInterface(0)

handle.interruptWrite(
	0x01,
	msg
);

handle.releaseInterface(0)
