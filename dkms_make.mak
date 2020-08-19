all:
	mkdir -p build
	$(MAKE) -C ./t150 all
	cp ./t150/t150.ko ./build
	$(MAKE) -C ./hid-tminit all
	cp ./hid-tminit/hid-tminit.ko ./build
