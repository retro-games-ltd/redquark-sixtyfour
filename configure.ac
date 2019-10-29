#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([the64], [1.1], [info@retrogames.biz])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AM_PROG_AR

AC_CHECK_TOOL(STRIP, strip, :)

# Checks for libraries.

AC_SUBST([ALSA_CFLAGS])
AC_SUBST([ALSA_LIBS])

PKG_CHECK_MODULES([ALSA],[alsa])
PKG_CHECK_MODULES([JPEG], [libjpeg], [
    AC_DEFINE([HAVE_LIBJPEG], 1, [Define to 1 if you have jpeg.])
], [
    AC_CHECK_LIB([jpeg], [jpeg_read_scanlines],
        AC_DEFINE([HAVE_LIBJPEG], 1, [Define to 1 if you have jpeg.]) AC_SUBST([JPEG_LIBS], [-ljpeg]),
        AC_MSG_ERROR([jpeg support requested but library not found])
    )]
)
PKG_CHECK_MODULES([PNG],[libpng16])
PKG_CHECK_MODULES([ZLIB],[zlib])
if test x"$LIBGCRYPT_CONFIG" = "x"; then
    AC_PATH_PROG(LIBGCRYPT_CONFIG, [libgcrypt-config], [libgcrypt-config])
fi
GCRYPT_CFLAGS="`${LIBGCRYPT_CONFIG} --cflags`"
GCRYPT_LIBS="`${LIBGCRYPT_CONFIG} --libs`"

AC_SUBST([GCRYPT_CFLAGS])
AC_SUBST([GCRYPT_LIBS])

AC_CHECK_FUNCS(getopt)
AC_CHECK_DECLS([getopt,getsubopt,getopt_long])
AC_REPLACE_FUNCS(getopt_long getsubopt)

RESID_USE_SSE=0

AC_SUBST([MAJOR], ["1"])
AC_SUBST([MINOR], ["3"])
AC_SUBST([PATCH], ["0"])

    AC_SUBST([PLATFORM], ["SUN8IW7"])
    AC_SUBST([CONF_CFLAGS], ["-march=armv7-a -mtune=cortex-a7 -fPIC"])
    dnl Wrap certain functions
    AC_SUBST([CONF_WFLAGS], ["-Wl,-wrap=bzero -Wl,-wrap=memcpy -Wl,-wrap=memset -Wl,-wrap=strcpy -Wl,-wrap=strlen -Wl,-wrap=strcmp"])

dnl resid-fp stuff
RESID_HAVE_BOOL=1
AC_SUBST(RESID_HAVE_BOOL)

if test x"$host_os" != "xminix" -a x"$host_os" != "xmint"; then
  AC_CHECK_FUNCS(logf expf)
fi

AC_MSG_CHECKING([if the logf prototype is present])
AC_TRY_COMPILE([#include <math.h>
                #include <stdio.h>],
               [printf("%d",logf);],
               [ AC_MSG_RESULT(yes)
                 HAVE_LOGF_PROTOTYPE=1
                ],
               [ AC_MSG_RESULT(no)
                HAVE_LOGF_PROTOTYPE=0
                ])

AC_MSG_CHECKING([if the expf prototype is present])
AC_TRY_COMPILE([#include <math.h>
                #include <stdio.h>],
               [printf("%d",expf);],
               [ AC_MSG_RESULT(yes)
                 HAVE_EXPF_PROTOTYPE=1
                ],
               [ AC_MSG_RESULT(no)
                HAVE_EXPF_PROTOTYPE=0
                ])

RESID_INLINE=inline
AC_SUBST(HAVE_LOGF_PROTOTYPE)
AC_SUBST(HAVE_EXPF_PROTOTYPE)
AC_SUBST(RESID_INLINE)
AC_SUBST(RESID_USE_SSE)
AC_OUTPUT(src/vice2/src/resid-fp/siddefs-fp.h)

dnl General outputs
AC_OUTPUT(
Makefile release.mk
src/viceport/Makefile
src/vbuild/Makefile.libc64cart
src/vbuild/Makefile.libc64cartsystem
src/vbuild/Makefile.libc64commoncart
src/vbuild/Makefile.libc64
src/vbuild/Makefile.libvic20
src/vbuild/Makefile.libvic20cart
src/vbuild/Makefile.libcore
src/vbuild/Makefile.libdiskimage
src/vbuild/Makefile.libdriveiecc64exp
src/vbuild/Makefile.libdriveiecieee
src/vbuild/Makefile.libdriveiec
src/vbuild/Makefile.libdriveieee
src/vbuild/Makefile.libdrive
src/vbuild/Makefile.libfileio
src/vbuild/Makefile.libfsdevice
src/vbuild/Makefile.libgfxoutputdrv
src/vbuild/Makefile.libiecbus
src/vbuild/Makefile.libimagecontents
src/vbuild/Makefile.libmonitor
src/vbuild/Makefile.libp64
src/vbuild/Makefile.libparallel
src/vbuild/Makefile.libplatform
src/vbuild/Makefile.libprinterdrv
src/vbuild/Makefile.libraster
src/vbuild/Makefile.libresidfp
src/vbuild/Makefile.libresid
src/vbuild/Makefile.librs232drv
src/vbuild/Makefile.librtc
src/vbuild/Makefile.libserial
src/vbuild/Makefile.libsid
src/vbuild/Makefile.libsounddrv
src/vbuild/Makefile.libtape
src/vbuild/Makefile.libuserport
src/vbuild/Makefile.libvdrive
src/vbuild/Makefile.libvicemain
src/vbuild/Makefile.libvicii
src/vbuild/Makefile.libvideo
src/vbuild/Makefile
src/c64emu/Makefile
src/vic20emu/Makefile
)
