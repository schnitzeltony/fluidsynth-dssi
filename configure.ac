dnl Process this file with autoconf to produce a configure script.

AC_INIT(FluidSynth-DSSI, 0.9.1, dssi-devel@lists.sourceforge.net)

AC_CONFIG_SRCDIR(src/fluidsynth-dssi.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_HEADER_STDC
AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_PROG_LIBTOOL

AC_C_BIGENDIAN

dnl check for other headers we need
AC_CHECK_HEADERS(math.h stdarg.h pthread.h dirent.h)

dnl Check for GTK+
AC_ARG_WITH(gtk2, AC_HELP_STRING([--with-gtk2], [use GTK+ 2.0 default=yes]),
            [ if test $withval = "yes"; then try_gtk2=yes ;
              else try_gtk2=no ; fi ], try_gtk2=yes)
with_gtk=no
dnl First look for GTK+ 2.x, unless '--without-gtk2' was specified.
if test "x${try_gtk2}" = 'xyes'; then
    AM_PATH_GTK_2_0(2.0.0, with_gtk='yes (version 2.0)', with_gtk=no)
fi
dnl  No GTK+ 2.x found, look for GTK+ 1.2 instead
if test "x${with_gtk}" = 'xno'; then
    AM_PATH_GTK(1.2.0, with_gtk='yes (version 1.2)', AC_MSG_WARN([GUI will not be built because no version of GTK+ was found]))
fi
AM_CONDITIONAL(BUILD_GUI, test "x${with_gtk}" != 'xno')
echo "GTK support: $with_gtk"

dnl Require ALSA or libdssialsacompat
PKG_CHECK_MODULES(ALSA, alsa, alsa=ALSA, alsa=no)
if test "x${alsa}" = xno ; then
  PKG_CHECK_MODULES(ALSA, libdssialsacompat, alsa=libdssialsacompat,
                    AC_MSG_ERROR([neither ALSA nor libdssialsacompat found]))
fi
if test "x${alsa}" = xALSA ; then
  unset ALSA_LIBS  # we only need the headers
fi

dnl Require DSSI, liblo, and fluidsynth
PKG_CHECK_MODULES(MODULE, dssi >= 0.9 liblo >= 0.12)
PKG_CHECK_MODULES(FLUID, fluidsynth >= 1.0.3)
AC_SUBST(FLUID_CFLAGS)
AC_SUBST(FLUID_LIBS)

dnl OS specific checks
case "${host_os}" in
darwin*)
  darwin=yes
  ;;
*)
  darwin=no
  ;;
esac
AM_CONDITIONAL(DARWIN, test x$darwin = xyes)

dnl Check for a patched libfluidsynth
AC_ARG_ENABLE(augmented-fluidsynth-api,
              AS_HELP_STRING([--enable-augmented-fluidsynth-api],
              [enable use of fluidsynth polyphony-setting API]),
              ,
              AC_CHECK_LIB(fluidsynth, fluid_synth_set_polyphony,
                           enable_augmented_fluidsynth_api=yes,
                           enable_augmented_fluidsynth_api=no, $FLUID_LIBS)
             )
if test "$enable_augmented_fluidsynth_api" = 'yes'; then
    echo "using augmented fluidsynth API"
    AC_DEFINE(USE_AUGMENTED_FLUIDSYNTH_API, 1, [Define to 1 if your libfluidsynth supports polyphony setting])
else
    echo "using standard fluidsynth API"
fi

dnl Check if we should assume fluid_synth_nwrite_float works correctly
AC_ARG_ENABLE(nwrite-float-workaround,
              AS_HELP_STRING([--disable-nwrite-float-workaround],
              [assume fluid_synth_nwrite_float() works correctly]),
              ,
              enable_nwrite_float_workaround=yes)
if test "x${enable_nwrite_float_workaround}" = 'xno'; then
    echo "assuming fluid_synth_nwrite_float() works correctly"
    AC_DEFINE(NWRITE_FLOAT_WORKS_CORRECTLY, 1, [Define to 1 if fluid_synth_nwrite_float() works correctly])
else
    echo "using fluid_synth_nwrite_float() workaround"
fi

dnl Use lotsa flags if we have gcc.
CFLAGS="$CFLAGS $ALSA_CFLAGS $MODULE_CFLAGS"
LDFLAGS="$LDFLAGS $ALSA_LIBS $MODULE_LIBS"
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
    *[\ \	]-Wall[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -Wall" ;;
  esac
  case " $CFLAGS " in
    *[\ \	]-O[1-9][\ \	]*) ;;
    *) CFLAGS="$CFLAGS -O2" ;;
  esac
  case " $CFLAGS " in
    *[\ \	]-ffast-math[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -Winline" ;;
  esac
fi
changequote([,])dnl
 
AC_OUTPUT([
Makefile
src/Makefile
])

