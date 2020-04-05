obj-m += ffb.o
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules
	cp ffb.ko ../build
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
