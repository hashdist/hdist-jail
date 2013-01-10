

build/hdistjail.c: src/hdistjail.c.in
	./runjinja.py src/hdistjail.c.in build/hdistjail.c

build/hdistjail.so: build build/hdistjail.c src/khash.h src/abspath.h
	gcc -Isrc -fPIC -Wall -c build/hdistjail.c -o build/hdistjail.o
	gcc -shared -o build/hdistjail.so build/hdistjail.o -ldl

build:
	mkdir build

jail: build/hdistjail.so

build/test_abspath: src/test_abspath.c src/abspath.h
	gcc -Wall -O0 -g -o build/test_abspath src/test_abspath.c

test_abspath: build/test_abspath
	./build/test_abspath

test: build/hdistjail.so
	python test_jail.py --nocapture -v

simpletest: build/hdistjail.so
	LD_PRELOAD=build/hdistjail.so HDIST_JAIL_LOG=jail.log cat hello

clean:
	rm build/*
