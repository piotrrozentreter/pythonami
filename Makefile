# Python68K — easy top-level builds
#
#   make host      Build host (Linux) debug executable → build/host/pythonami
#   make amiga     Build AmigaOS 68000 release Hunk → ./pythonami
#   make test      Host unit + language tests
#   make clean     Remove host and Amiga build products
#
# Optional overrides:
#   make host MODE=release
#   make amiga MODE=debug
#   make amiga VBCC=/path/to/vbcc NDK=/path/to/NDK3.2

VBCC ?= /home/piotr/local/vbcc
NDK  ?= /run/media/piotr/BACKUP/Rozen/Programy/Amiga/NDK3.2

# Optional: MODE=debug|release
#   make host   → debug unless MODE=release
#   make amiga  → release unless MODE=debug
MODE ?=

.PHONY: all help host amiga debug release test language-test \
	amiga-debug amiga-release clean

all: help

help:
	@echo "Python68K build targets:"
	@echo "  make host              Host GCC build (default debug; MODE=release for optimized)"
	@echo "  make amiga             Amiga vbcc +aos68k build (default release; MODE=debug for debug)"
	@echo "  make test              Host unit + language tests"
	@echo "  make language-test     Host language fixture diffs only"
	@echo "  make clean             Remove build products"
	@echo ""
	@echo "Aliases: debug, release, amiga-debug, amiga-release"
	@echo "Amiga paths: VBCC=$(VBCC)"
	@echo "             NDK=$(NDK)"

# ---- Host (GCC) ----------------------------------------------------------

ifeq ($(MODE),release)
host:
	$(MAKE) -f Makefile.host release
else
host:
	$(MAKE) -f Makefile.host debug
endif
	@echo "Host binary: build/host/pythonami"

debug release test language-test:
	"$(MAKE)" -f Makefile.host $@

# ---- Amiga (vbcc) --------------------------------------------------------

amiga:
	@if [ "$(MODE)" = "debug" ]; then \
		"$(MAKE)" -f Makefile.amiga amiga-debug VBCC="$(VBCC)" NDK="$(NDK)"; \
		echo "Amiga binary: pythonami-debug"; \
	else \
		"$(MAKE)" -f Makefile.amiga amiga-release VBCC="$(VBCC)" NDK="$(NDK)"; \
		echo "Amiga binary: pythonami"; \
	fi

amiga-debug:
	"$(MAKE)" -f Makefile.amiga amiga-debug VBCC="$(VBCC)" NDK="$(NDK)"

amiga-release:
	"$(MAKE)" -f Makefile.amiga amiga-release VBCC="$(VBCC)" NDK="$(NDK)"

# ---- Clean ---------------------------------------------------------------

clean:
	"$(MAKE)" -f Makefile.host clean
	"$(MAKE)" -f Makefile.amiga clean
