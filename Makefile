
CC = gcc
INSTALL = install
PREFIX = /usr/local

CFLAGS += -Wall -fPIC -Isrc
LDFLAGS += -shared -ldl

OBJ = build/faketime.o

SONAME = 1
LIBS = build/libhdistjail.so.${SONAME}

all: build ${LIBS}

build/hdistjail.c: src/hdistjail.c.in
	./runjinja.py $< $@

build/hdistjail.o: build/hdistjail.c
	${CC} -o $@ -c ${CFLAGS} $<

build/libhdistjail.so.${SONAME}: build/hdistjail.o
	${CC} -o $@ -Wl,-soname,libhdistjail.so.${SONAME} $< ${LDFLAGS}

clean:
	@rm -rf build

distclean: clean
	@echo

install: ${LIBS}
	@echo
	$(INSTALL) -dm0755 "${DESTDIR}${PREFIX}/lib/"
	$(INSTALL) -m0644 ${LIBS} "${DESTDIR}${PREFIX}/lib/"

test: build/hdistjail.so
	python test_jail.py --nocapture -v

simpletest: build/hdistjail.so
	LD_PRELOAD=build/hdistjail.so HDIST_JAIL_LOG=jail.log cat hello

build:
	mkdir build

.PHONY: all clean distclean install test simpletest

