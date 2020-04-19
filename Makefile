all:
	mkdir -p build
	$(MAKE) -C ./t150 all
	$(MAKE) -C ./ffb all
