prefix = /usr/local
all:src/prown

src/prown:src/prown.c
	$(CC) -o $@ $^ -lbsd

install: src/prown
	install -D src/prown \
                $(DESTDIR)$(prefix)/bin/prown

clean:
	-rm -f src/prown tests/isolate

tests/isolate: tests/isolate.c
	$(CC) -Wall -o $@ $^

tests: src/prown tests/isolate
	tests/run.sh

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/prown

.PHONY: all install clean distclean uninstall tests
