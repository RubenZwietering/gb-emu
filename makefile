INCDIR ?= inc/
SRCDIR ?= src/
BINDIR ?= bin/

INCS :=
SRCS := \
	main.cpp \
	apu.cpp  \
	bus.cpp  \
	cgb.cpp  \
	cpu.cpp  \
	cpu_instructions.cpp \
	cpu_instruction_name_table.cpp \
	dmg.cpp  \
	mem.cpp  \
	ppu.cpp
BIN ?= gb-emu

SRC_PATHS = $(patsubst %,$(SRCDIR)%,$(SRCS))

.PHONY: all run clean

all: $(BINDIR) $(BINDIR)$(BIN)

$(BINDIR):
	mkdir $@

$(BINDIR)$(BIN): linux windows

linux: $(SRC_PATHS)
	gcc -O2 -Wall -std=c++20 -lstdc++ -o $(BINDIR)$(BIN) $^

windows: $(SRC_PATHS)
	setup-and-cl32 /nologo /EHsc /W3 /std:c++20 /Fo:$(BINDIR) $^ /link /out:$(BINDIR)$(BIN).exe
	chmod +x $(BINDIR)$(BIN).exe

run-linux:
	$(BINDIR)$(BIN)

run-windows:
	$(BINDIR)$(BIN).exe

clean:
	rm -f $(BINDIR)*
	rmdir $(BINDIR)
