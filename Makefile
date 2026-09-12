.PHONY: debug release test clean amiga-debug amiga-release

debug release test clean:
	$(MAKE) -f Makefile.host $@

amiga-debug amiga-release:
	$(MAKE) -f Makefile.amiga $@
