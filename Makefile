all:
	make menu
	make exploit

exploit:
	cd payload; make; rm ../stage2/payload.bin.gz; gzip -9 -n -c payload.bin > ../stage2/payload.bin.gz
	cd stage2; make
	cd stage1; make
	./scripts/obfuscate.py stage1/stage1.bin 1
	./scripts/obfuscate.py stage2/stage2.bin 2
	./scripts/trim.py stage2/stage2.bin
	./scripts/generate.py stage1/stage1.bin stage2/stage2.bin system.dat

menu:
	cd bootstrap; mkdir build; cd build; cmake ..; make; xxd -i bootstrap.self > ../../payload/bootstrap.h

clean:
	-rm system.dat
	-rm -rf bootstrap/build
	-rm payload/bootstrap.h
	-rm stage2/payload.bin.gz
	cd payload; make clean
	cd stage2; make clean
	cd stage1; make clean
