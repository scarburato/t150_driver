all:
	mkdir -p build
	$(MAKE) -C ./hid-t150 all
	cp ./hid-t150/hid-t150.ko ./build
	$(MAKE) -C ./hid-tminit all
	cp ./hid-tminit/hid-tminit.ko ./build
	
clean:
	$(MAKE) -C ./hid-t150 clean
	$(MAKE) -C ./hid-tminit clean
	rm -r ./build/*
