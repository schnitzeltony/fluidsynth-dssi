# fluidsynth-dssi Makefile
# ========================

# Edit things above the long horizontal line, as appropriate for
# your system, then do 'make' and 'make install'.

# 1. Define the following to point to the unpacked fluidsynth source:

FLUID ?= ../../fluidsynth-1.0.3
FLUID_SRC = $(FLUID)/src
FLUID_INCLUDE = $(FLUID)/include

# 2. Set this to where you want fluidsynth-dssi installed:

PREFIX ?= /usr/local

# 3. Adjust anything here as needed:

OPTIMIZATION_CFLAGS = -O2 -s -fomit-frame-pointer -funroll-all-loops -ffast-math -finline-functions -finline-limit=5000 -minline-all-stringops -Winline

PLUGIN_CFLAGS = -Wall -I. -I../dssi -I$(FLUID_INCLUDE) -I$(FLUID_SRC) $(OPTIMIZATION_CFLAGS)
PLUGIN_LDFLAGS = -nostartfiles -shared
PLUGIN_LDLIBS = -lfluidsynth

GUI_CFLAGS = -Wall -O2 $(shell gtk-config --cflags) $(shell pkg-config liblo --cflags) -I$(FLUID_INCLUDE) -I$(FLUID_SRC)
GUI_LDFLAGS = $(shell gtk-config --libs) $(shell pkg-config liblo --libs) -lfluidsynth

TARGETS	=	fluidsynth-dssi.so FluidSynth-DSSI_gtk

# ----------------------------------------------------------------------------

all: $(TARGETS)

install: all
	mkdir -p $(PREFIX)/lib/dssi/fluidsynth-dssi
	cp fluidsynth-dssi.so $(PREFIX)/lib/dssi/
	cp FluidSynth-DSSI_gtk $(PREFIX)/lib/dssi/fluidsynth-dssi/
	test -x $(PREFIX)/bin/jack-dssi-host && ln -s jack-dssi-host $(PREFIX)/bin/fluidsynth-dssi

clean:
	rm -f *.o

distclean:	clean
	rm -f *~ $(TARGETS)

fluidsynth-dssi.o: fluidsynth-dssi.c fluidsynth-dssi.h ../dssi/dssi.h
	gcc $(PLUGIN_CFLAGS) -c -o fluidsynth-dssi.o fluidsynth-dssi.c

fluidsynth-dssi.so: fluidsynth-dssi.o
	gcc $(PLUGIN_LDFLAGS) -o fluidsynth-dssi.so fluidsynth-dssi.o $(PLUGIN_LDLIBS)

FluidSynth-DSSI_gtk: FluidSynth-DSSI_gtk.c FluidSynth-DSSI_gtk.h ../dssi/dssi.h
	gcc $(GUI_CFLAGS) -o FluidSynth-DSSI_gtk FluidSynth-DSSI_gtk.c $(GUI_LDFLAGS)


