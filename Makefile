

build/hdistjail.so: build src/hdistjail.c src/khash.h src/abspath.h
	gcc -fPIC -Wall -c src/hdistjail.c -o build/hdistjail.o
	gcc -shared -o build/hdistjail.so build/hdistjail.o -ldl

build:
	mkdir build

test: build/hdistjail.so
	LD_PRELOAD=build/hdistjail.so HDIST_JAIL_WHITELIST=whitelist cat foo
clean:
	rm build/*
