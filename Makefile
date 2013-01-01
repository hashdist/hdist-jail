

build/hdistrestrict.so: build src/hdistrestrict.c
	gcc -fPIC -Wall -c src/hdistrestrict.c -o build/hdistrestrict.o
	gcc -shared -o build/hdistrestrict.so build/hdistrestrict.o -ldl

build:
	mkdir build

clean:
	rm build/*
