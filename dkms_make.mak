export KDIR

all:
	mkdir -p build
	$(MAKE) -C ./hid-t150 all
	cp ./hid-t150/hid-t150.ko ./build
	
clean:
	$(MAKE) -C ./hid-t150 clean
	rm -rf ./build/*
