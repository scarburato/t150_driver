obj-m += t150.o
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

allCopyNClean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules
	cp t150.ko ../build
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
