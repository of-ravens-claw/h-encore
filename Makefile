export FW

ifeq ($(strip $(FW)),)
  ifeq ($(filter clean menu,$(MAKECMDGOALS)),)
    $(error Error: FW is not defined. Please run 'make FW=<version>' (e.g., make FW=363))
  endif
endif

all:
	$(MAKE) menu
	$(MAKE) exploit

exploit:
	cd payload; $(MAKE); rm ../stage2/payload.bin.gz; gzip -9 -n -c payload.bin > ../stage2/payload.bin.gz
	cd stage2; $(MAKE)
	cd stage1; $(MAKE)
#	./scripts/obfuscate.py stage1/stage1.bin 1
#	./scripts/obfuscate.py stage2/stage2.bin 2
	./scripts/trim.py stage2/stage2.bin
	./scripts/generate.py stage1/stage1.bin stage2/stage2.bin system.dat

menu:
	cd bootstrap; mkdir build; cd build; cmake ..; $(MAKE); xxd -i bootstrap.self > ../../payload/bootstrap.h

clean:
	-rm system.dat
	-rm -rf bootstrap/build
	-rm payload/bootstrap.h
	-rm stage2/payload.bin.gz
	cd payload; $(MAKE) clean
	cd stage2; $(MAKE) clean
	cd stage1; $(MAKE) clean
