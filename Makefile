# fluidsynth-dssi v0.1.0 Makefile
# ===============================

# Edit things above the long horizontal line, as appropriate for
# your system, then do 'make'.  There is no 'make install' yet.

# 1. Define the following to point to the unpacked fluidsynth source:

FLUID_SRC=/home/cannam/fluidsynth-1.0.1/src

# 2. If you're bulding on OS X, comment out the 'linux' section, and
#    uncomment the 'darwin' section:

# linux:
# ======
OPTIMIZATION_CFLAGS = -O2 -s -fomit-frame-pointer -funroll-all-loops -ffast-math -finline-functions -finline-limit=5000 -minline-all-stringops -Winline
PLUGIN_CFLAGS = -Wall -I. -I../dssi -I$(FLUID_SRC) $(OPTIMIZATION_CFLAGS)
PLUGIN_LDFLAGS = -nostartfiles -shared
PLUGIN_LDLIBS = -lfluidsynth 
GUI_CFLAGS = -Wall -O2 $(shell gtk-config --cflags) -I/opt/audio/include -I$(FLUID_SRC)
GUI_LDFLAGS = $(shell gtk-config --libs) -L/opt/audio/lib -llo -lfluidsynth

# darwin:
# =======
#DARWIN_EXTRA_CFLAGS = -I/Users/smbolton/_dssi/alsa-lib-0.9.8/include -I/Users/smbolton/_dssi/include -I/Users/smbolton/_dssi/include/alsa
#DARWIN_EXTRA_PLUGIN_CFLAGS = $(DARWIN_EXTRA_CFLAGS) -I../fluidsynth-1.0.3/include
#OPTIMIZATION_CFLAGS = -O2 -s -fomit-frame-pointer -funroll-all-loops -ffast-math -finline-functions -finline-limit=5000 -Winline
#PLUGIN_CFLAGS = -fno-common -Wall -I. $(DARWIN_EXTRA_PLUGIN_CFLAGS) -I$(FLUID_SRC) $(OPTIMIZATION_CFLAGS)
#PLUGIN_LDFLAGS = -bundle -flat_namespace -undefined suppress
#PLUGIN_LDLIBS = /Users/smbolton/src/fluidsynth/fluidsynth-1.0.3-darwin/src/.libs/libfluidsynth.a -lc
# !FIX! GUI flags

# ----------------------------------------------------------------------------

all: fluidsynth-dssi.so FluidSynth-DSSI_gtk

fluidsynth-dssi.o: fluidsynth-dssi.c fluidsynth-dssi.h ../dssi/dssi.h
	gcc $(PLUGIN_CFLAGS) -c -o fluidsynth-dssi.o fluidsynth-dssi.c

fluidsynth-dssi.so: fluidsynth-dssi.o
	gcc $(PLUGIN_LDFLAGS) -o fluidsynth-dssi.so fluidsynth-dssi.o $(PLUGIN_LDLIBS)

FluidSynth-DSSI_gtk: FluidSynth-DSSI_gtk.c FluidSynth-DSSI_gtk.h ../dssi/dssi.h
	gcc $(GUI_CFLAGS) -o FluidSynth-DSSI_gtk FluidSynth-DSSI_gtk.c $(GUI_LDFLAGS)


