all:
	mkdir -p build
	$(MAKE) -C ./t150 all
	$(MAKE) -C ./thrustmaster_enable_full all
