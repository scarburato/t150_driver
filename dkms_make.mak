all:
	mkdir -p build
	$(MAKE) -C ./hid-t150 all
	cp ./hid-t150/hid-t150.ko ./build
	$(MAKE) -C ./hid-tminit all
	cp ./hid-tminit/hid-tminit.ko ./build
