# DMVGRAF - Simulation am PC (gcc). Unter CP/M-8000: test/GTEST.SUB.
CC     = gcc
CFLAGS = -x c -DHOST -std=gnu89 -O1 -g -w -Isrc
LIB    = src/GRAF.C src/GBILD.C src/GEIN.C src/GKERN.C src/GHWSIM.C

gtest: test/GTEST.C $(LIB) src/*.H
	$(CC) $(CFLAGS) -o $@ test/GTEST.C $(LIB)

clean:
	rm -f gtest *.ppm
