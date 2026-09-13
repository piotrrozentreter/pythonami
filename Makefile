.PHONY: debug release test clean amiga-debug amiga-release

# GNU Make on Windows may expose its executable through a path containing
# spaces. Quote the recursive invocation so the host and Amiga wrappers remain
# callable from PowerShell.
MAKE := "$(MAKE)"

debug release test clean:
	$(MAKE) -f Makefile.host $@

amiga-debug amiga-release:
	$(MAKE) -f Makefile.amiga $@
