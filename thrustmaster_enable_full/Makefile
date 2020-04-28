obj-m += thrustmaster_enable_full.o
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules
	cp thrustmaster_enable_full.ko ../build
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
