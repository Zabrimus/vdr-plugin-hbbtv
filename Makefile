#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = hbbtv

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).cpp | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:../../.." pkg-config --variable=$(1) vdr))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
CONFDEST = $(call PKGCFG,configdir)/plugins/$(PLUGIN)

TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES +=

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o ait.o hbbtvurl.o hbbtvmenu.o status.o cefhbbtvpage.o osddispatcher.o hbbtvvideocontrol.o \
		browsercommunication.o globals.o sharedmemory.o

SRCS = $(wildcard $(OBJS:.o=.cpp)) $(PLUGIN).cpp

### libraries

# nng
NNGCFLAGS  = -Ithirdparty/nng-1.2.6/include/nng/compat
NNGLDFLAGS = thirdparty/nng-1.2.6/build/libnng.a

# ffmpeg libswscale
CXXFLAGS += $(shell pkg-config --cflags libswscale)
LDFLAGS += $(shell pkg-config --libs libswscale)

### The main target:

all:
	$(MAKE) buildnng
	$(MAKE) $(SOFILE)
	$(MAKE) i18n

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $(NNGCFLAGS) -o $@ $<

%.o: %.c
	@echo CC $@
	$(Q)$(CC) $(CFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR	  = po
I18Npo	  = $(wildcard $(PODIR)/*.po)
I18Nmo	  = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot	  = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	@echo MO $@
	$(Q)msgfmt -c -o $@ $<

$(I18Npot): $(SRCS)
	@echo GT $@
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP \
	-k_ -k_N --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) \
	--msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	@echo PO $@
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

buildnng:
ifneq (exists, $(shell test -e thirdparty/nng-1.2.6/build/libnng.a && echo exists))
	mkdir -p thirdparty/nng-1.2.6/build && \
	cd thirdparty/nng-1.2.6/build && \
	cmake .. && \
	$(MAKE)
endif

$(SOFILE): buildnng $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LDFLAGS) $(LIBS) $(NNGLDFLAGS) -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n install-config
	echo ${CONFDEST}

install-config:
	if ! test -d $(DESTDIR)$(CONFDEST); then \
		mkdir -p $(DESTDIR)$(CONFDEST); \
		chmod a+rx $(DESTDIR)$(CONFDEST); \
	fi
	install --mode=644 -D ./config/* $(DESTDIR)$(CONFDEST)
ifdef VDR_USER
	if test -n $(VDR_USER); then \
		chown $(VDR_USER) $(DESTDIR)$(CONFDEST); \
		chown $(VDR_USER) $(DESTDIR)$(CONFDEST)/*; \
	fi
endif

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
	@-rm -rf thirdparty/nng-1.2.6/build