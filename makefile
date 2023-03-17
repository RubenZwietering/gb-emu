INCDIR ?= inc/
SRCDIR ?= src/
BINDIR ?= bin/

INCS :=
SRCS := \
	main.cpp
BIN ?= gb-emu

SRC_PATHS = $(patsubst %,$(SRCDIR)%,$(SRCS))

.PHONY: all run clean

all: $(BINDIR) $(BINDIR)$(BIN)

$(BINDIR):
	mkdir $@

$(BINDIR)$(BIN): linux windows

linux: $(SRC_PATHS)
	gcc -O2 -Wall -lstdc++ -o $(BINDIR)$(BIN) $^

windows: $(SRC_PATHS)
	setup-and-cl32 /nologo /EHsc /W3 /Fo:$(BINDIR) $^ /link /out:$(BINDIR)$(BIN).exe
	chmod +x $(BINDIR)$(BIN).exe

run-linux:
	$(BINDIR)$(BIN)

run-windows:
	$(BINDIR)$(BIN).exe

clean:
	rm -f $(BINDIR)*
	rmdir $(BINDIR)
