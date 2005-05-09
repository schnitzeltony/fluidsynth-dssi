## Process this file with automake to produce Makefile.in

if BUILD_GUI
ui_PROGRAMS = FluidSynth-DSSI-091a2_gtk
else
ui_PROGRAMS =
endif

uidir = $(libdir)/dssi/fluidsynth-dssi-091a2

plugindir = $(libdir)/dssi

plugin_LTLIBRARIES = fluidsynth-dssi-091a2.la

FluidSynth_DSSI_091a2_gtk_SOURCES = \
	FluidSynth-DSSI_gtk.c \
	load_soundfont_metadata.c \
	load_soundfont_metadata.h \
	locate_soundfont.c

FluidSynth_DSSI_091a2_gtk_CFLAGS = @GTK_CFLAGS@ $(AM_CFLAGS)

if DARWIN
FluidSynth_DSSI_091a2_gtk_LDADD = -lm -lmx @GTK_LIBS@ $(AM_LDFLAGS)
else
FluidSynth_DSSI_091a2_gtk_LDADD = -lm @GTK_LIBS@ $(AM_LDFLAGS)
endif

fluidsynth_dssi_091a2_la_SOURCES = \
	fluidsynth-dssi.c \
	fluidsynth-dssi.h \
	locate_soundfont.c

fluidsynth_dssi_091a2_la_CFLAGS = @FLUID_CFLAGS@

if DARWIN
fluidsynth_dssi_091a2_la_LDFLAGS = -module -avoid-version
else
fluidsynth_dssi_091a2_la_LDFLAGS = -module -avoid-version -Wc,-nostartfiles
endif

fluidsynth_dssi_091a2_la_LIBADD = -lm @FLUID_LIBS@

