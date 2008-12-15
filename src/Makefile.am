## Process this file with automake to produce Makefile.in

if BUILD_GUI
ui_PROGRAMS = FluidSynth-DSSI_gtk
else
ui_PROGRAMS =
endif

uidir = $(libdir)/dssi/fluidsynth-dssi

plugindir = $(libdir)/dssi

plugin_LTLIBRARIES = fluidsynth-dssi.la

FluidSynth_DSSI_gtk_SOURCES = \
	FluidSynth-DSSI_gtk.c \
	load_soundfont_metadata.c \
	load_soundfont_metadata.h \
	locate_soundfont.c

FluidSynth_DSSI_gtk_CFLAGS = @GTK_CFLAGS@ $(AM_CFLAGS)

if DARWIN
FluidSynth_DSSI_gtk_LDADD = -lm -lmx @GTK_LIBS@ $(AM_LDFLAGS)
else
FluidSynth_DSSI_gtk_LDADD = -lm @GTK_LIBS@ $(AM_LDFLAGS)
endif

fluidsynth_dssi_la_SOURCES = \
	fluidsynth-dssi.c \
	fluidsynth-dssi.h \
	locate_soundfont.c

fluidsynth_dssi_la_CFLAGS = @FLUID_CFLAGS@

fluidsynth_dssi_la_LDFLAGS = -module -avoid-version

fluidsynth_dssi_la_LIBADD = -lm @FLUID_LIBS@

# create symlinks for the plugin to jack-dssi-host
install-exec-hook:
	if test -x $(DESTDIR)$(bindir)/jack-dssi-host$(EXEEXT) ; then \
	  cd $(DESTDIR)$(bindir) ; \
	  if ! test -x ./fluidsynth-dssi$(EXEEXT) ; then \
	    $(LN_S) jack-dssi-host$(EXEEXT) fluidsynth-dssi$(EXEEXT) ; \
	  fi ; \
	fi

