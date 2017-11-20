dnl Process this file with autoconf to produce a configure script.

AC_INIT(FluidSynth-DSSI, 1.0.0, dssi-devel@lists.sourceforge.net)

AC_CONFIG_SRCDIR(src/fluidsynth-dssi.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_PROG_LIBTOOL

AC_C_BIGENDIAN

dnl check for other headers we need
AC_CHECK_HEADERS(math.h stdarg.h pthread.h dirent.h ladspa.h)

dnl Check for FluidSynth
PKG_CHECK_MODULES(FLUID, fluidsynth >= 1.1.6)
AC_SUBST(FLUID_CFLAGS)
AC_SUBST(FLUID_LIBS)

dnl Check for glib
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.40)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl Check for GTK+
AC_ARG_WITH(gtk2, AC_HELP_STRING([--with-gtk2], [use GTK+ 2.0 default=yes]),
            [ if test $withval = "yes"; then try_gtk2=yes ;
              else try_gtk2=no ; fi ], try_gtk2=yes)
with_gtk=no
PKG_CHECK_MODULES(GTK, gtk+-2.0, with_gtk=yes, with_gtk=no)
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

dnl Require DSSI, liblo
PKG_CHECK_MODULES(MODULE, dssi >= 0.9 liblo >= 0.12)

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

