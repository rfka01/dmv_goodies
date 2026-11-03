# DMVGRAF - Simulation am PC (gcc), CP/M-68K-Fassung (m68k-linux-gnu-gcc)
# und ihr Pruefstand. Unter CP/M-8000: test/GTEST.SUB.
CC     = gcc
CFLAGS = -x c -DHOST -std=gnu89 -O1 -g -w -Isrc
LIB    = src/GRAF.C src/GBILD.C src/GEIN.C src/GKERN.C src/GHWSIM.C src/GPIC.C

gtest: test/GTEST.C $(LIB) src/*.H
	$(CC) $(CFLAGS) -o $@ test/GTEST.C $(LIB)

stest: test/STEST.C $(LIB) src/*.H
	$(CC) $(CFLAGS) -o $@ test/STEST.C $(LIB)

ptest: test/PTEST.C $(LIB) src/*.H
	$(CC) $(CFLAGS) -o $@ test/PTEST.C $(LIB)

pictest: test/PICTEST.C $(LIB) src/*.H
	$(CC) $(CFLAGS) -o $@ test/PICTEST.C $(LIB)

btest: test/BTEST.C $(LIB) src/*.H
	$(CC) $(CFLAGS) -o $@ test/BTEST.C $(LIB)

LIB68  = src/GCRT68.S src/GSUP68.S src/GARG68.C src/GRAF.C src/GBILD.C \
         src/GEIN.C src/GK68.S src/GHWCPM.C src/GHD68K.C src/GHPOOL.C src/GPIC.C

# CP/M-68K (K234): GCRT68.S muss vorn stehen
GTEST.68K: test/GTEST.C $(LIB68) src/*.H
	python3 tools/bau68.py -I src -D GBPOOL=61440 -o $@ \
		$(firstword $(LIB68)) test/GTEST.C $(wordlist 2,99,$(LIB68))

MTEST.68K: test/MTEST.C $(LIB68) src/*.H
	python3 tools/bau68.py -I src -D GBPOOL=61440 -o $@ \
		$(firstword $(LIB68)) test/MTEST.C $(wordlist 2,99,$(LIB68))

# MS-DOS (8088/V20, small) mit gcc-ia16 (/opt/ia16/bin oder IA16=...)
LIBDOS = src/GARG68.C src/GRAF.C src/GBILD.C src/GEIN.C src/GKERN.C src/GHWDOS.C \
         src/GINFO.C src/GZEIT.C src/GPIC.C src/GPIC86.S

GTEST.EXE: test/GTEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/GTEST.C $(LIBDOS)

# Speichermodell tiny (.COM)
GTEST.COM: test/GTEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/GTEST.C $(LIBDOS)

STEST.EXE: test/STEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/STEST.C $(LIBDOS)

PTEST.EXE: test/PTEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/PTEST.C $(LIBDOS)

BTEST.EXE: test/BTEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/BTEST.C $(LIBDOS)

ITEST.COM: test/ITEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/ITEST.C $(LIBDOS)

PICTEST.EXE: test/PICTEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/PICTEST.C $(LIBDOS)

MTEST.EXE: test/MTEST.C src/GCRTDOS.S $(LIBDOS) src/*.H
	python3 tools/baudos.py -I src -o $@ src/GCRTDOS.S test/MTEST.C $(LIBDOS)

# fertige Bibliotheken (lib/README.md); CP/M-8000 entsteht mit ar8k
lib: $(LIBDOS) $(LIB68) src/*.H
	python3 tools/baudos.py -I src -o lib/dos/DMVGRAF.A $(LIBDOS)
	python3 tools/baudos.py -o lib/dos/GCRTDOS.O src/GCRTDOS.S
	python3 tools/baudos.py --tiny -I src -o lib/dos-tiny/DMVGRAF.A $(LIBDOS)
	python3 tools/baudos.py --tiny -o lib/dos-tiny/GCRTCOM.O src/GCRTDOS.S
	python3 tools/bau68.py -I src -D GBPOOL=61440 -o lib/cpm68k/DMVGRAF.A \
		$(wordlist 2,99,$(LIB68))
	python3 tools/bau68.py -o lib/cpm68k/GCRT68.O src/GCRT68.S

.PHONY: lib clean

# Pruefstand: python3 tools/pruef68.py GTEST.68K skript.txt [--block]
simlib.so: tools/simlib.c src/GHWSIM.C src/*.H
	$(CC) -shared -fPIC -O1 -w -x c -DHOST -std=gnu89 -Isrc -o tools/$@ tools/simlib.c

clean:
	rm -f gtest stest ptest btest pictest GTEST.68K MTEST.68K *.EXE *.COM tools/simlib.so *.ppm
