MAINSRC     := $(shell pwd)
VICE        ?= $(MAINSRC)/vice2
FREETYPE_GL ?= $(MAINSRC)/freetype-gl

include ../release.mk

CARG := VICE=$(VICE) FREETYPE_GL=$(FREETYPE_GL) MAINSRC=$(MAINSRC) MAJOR=$(MAJOR) MINOR=$(MINOR) PATCH=$(PATCH)

TARGET=all

all: build

build:
	@$(MAKE) -C viceport $(CARG)
	@$(MAKE) -C vbuild   $(CARG)
	@$(MAKE) -C c64emu   $(CARG)
	@$(MAKE) -C vic20emu $(CARG)

clean:
	@$(MAKE) -C viceport $(CARG) clean
	@$(MAKE) -C vbuild   $(CARG) clean
	@$(MAKE) -C c64emu   $(CARG) clean
	@$(MAKE) -C vic20emu $(CARG) clean
