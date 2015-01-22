CC=gcc
CFLAGS=-m64 -O3 -ffast-math -funroll-loops -march=native
%CFLAGS=-g

all: scenario_ranking bench

scenario_ranking: scenario_ranking.c

bench: bench.c

clean:
	rm scenario_ranking bench
