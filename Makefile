

build/hdistjail.so: build src/hdistjail.c src/khash.h src/abspath.h
	gcc -fPIC -Wall -c src/hdistjail.c -o build/hdistjail.o
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

clean:
	rm build/*
