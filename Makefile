CFLAGS ?= -Wall
EXEC = prown
PROWN_SRC = $(wildcard src/*.c)
TESTS_SRC = $(wildcard tests/*.c)
SRC = $(PROWN_SRC) $(TESTS_SRC)
INDENT_FLAGS = --no-tabs \
               --indent-level4 \
               --braces-on-if-line \
               --cuddle-else \
               --continue-at-parentheses \
               --dont-break-procedure-type \
               --no-space-after-function-call-names \
               --braces-on-func-def-line \
               --blank-lines-after-declarations
CHECK = cppcheck
CHECKFLAGS ?= -I /usr/include -I /usr/include/linux --enable=all --language=c
LANG_MO = po/fr.mo
MANPAGE = doc/man/$(EXEC).1
BIN = src/$(EXEC)
prefix = /usr/local

all: $(BIN) $(MANPAGE) $(LANG_MO)

$(BIN): $(PROWN_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lbsd -lacl

po/%.mo: po/%.po
	msgfmt --output-file=$@ $<

po/%.po: po/$(EXEC).pot
	msgmerge --update $@ $<

po/$(EXEC).pot: $(PROWN_SRC)
	xgettext --keyword=_ --language=C --add-comments --sort-output --package-name=$(EXEC) --output $@ $(PROWN_SRC)

install: src/prown
	install -D -m 755 $(BIN) $(DESTDIR)$(prefix)/bin/$(EXEC)
	$(foreach _MO,$(LANG_MO),install -D -m 644 $(_MO) $(DESTDIR)$(prefix)/share/locale/$(notdir $(basename $(_MO)))/LC_MESSAGES/$(EXEC).mo)
	install -D -m 644 $(MANPAGE) $(DESTDIR)$(prefix)/share/man/man1/$(EXEC).1

%.1: %.1.md
	pandoc --standalone --from markdown --to=man $^ --output $@

clean:
	-rm -f $(BIN) $(MANPAGE) tests/isolate po/*~ po/*.mo

indent:
	indent $(INDENT_FLAGS) $(SRC)
	sed -i 's/ *$$//;' $(SRC)  # remove trailing whitespaces

check:
	$(CHECK) $(CHECKFLAGS) $(PROWN_SRC)

tests/isolate: $(TESTS_SRC)
	$(CC) $(CFLAGS) -o $@ $^

tests: src/prown tests/isolate
	tests/run.sh

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)$(BIN)

.PHONY: all po doc install clean distclean indent check uninstall tests
