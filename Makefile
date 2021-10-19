CFLAGS ?= -Wall
PROWN_SRC = $(wildcard src/*.c)
TESTS_SRC = $(wildcard tests/*.c)
SRC = $(PROWN_SRC) $(TESTS_SRC)
prefix = /usr/local
all:src/prown

src/prown: $(PROWN_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lbsd

install: src/prown
	install -D src/prown \
                $(DESTDIR)$(prefix)/bin/prown

clean:
	-rm -f src/prown tests/isolate

tests/isolate: $(TESTS_SRC)
	$(CC) $(CFLAGS) -o $@ $^

tests: src/prown tests/isolate
	tests/run.sh

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/prown

.PHONY: all install clean distclean uninstall tests
