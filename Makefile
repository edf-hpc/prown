prefix = /usr/local
all:src/prown

src/prown:src/prown.c
	$(CC)  -o $@ $^

install: src/prown
	install -D src/prown \
                $(DESTDIR)$(prefix)/bin/prown

clean:
	-rm -f src/prown

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/prown

.PHONY: all install clean distclean uninstall 
