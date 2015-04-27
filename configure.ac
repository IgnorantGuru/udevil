AC_PREREQ(2.52)

AC_INIT(src/udevil.c)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(src)
AM_INIT_AUTOMAKE(udevil, 0.4.3+)

AC_PROG_INTLTOOL([0.21])

AM_MAINTAINER_MODE
AC_ISC_POSIX
AM_PROG_CC_C_O
AC_HEADER_STDC
AM_PROG_LIBTOOL
AC_PROG_INSTALL

AC_DEFINE_UNQUOTED(UDEVIL_VERSION, "$VERSION", [udevil version])

# sysconfdir defaults to /usr/local/etc if this is not done:
test "$sysconfdir" = '${prefix}/etc' && sysconfdir=/etc

dnl linker optimizations
AC_MSG_CHECKING([whether $LD accepts --as-needed])
case `$LD --as-needed -v 2>&1 </dev/null` in
*GNU* | *'with BFD'*)
  LDFLAGS="$LDFLAGS -Wl,--as-needed"
  AC_MSG_RESULT([yes])
  ;;
*)
  AC_MSG_RESULT([no])
  ;;
esac
AC_MSG_CHECKING([whether $LD accepts -O1])
case `$LD -O1 -v 2>&1 </dev/null` in
*GNU* | *'with BFD'*)
  LDFLAGS="$LDFLAGS -Wl,-O1"
  AC_MSG_RESULT([yes])
  ;;
*)
  AC_MSG_RESULT([no])
  ;;
esac
AC_MSG_CHECKING([whether $LD accepts -Bsymbolic-functions])
case `$LD -Bsymbolic-functions -v 2>&1 </dev/null` in
*GNU* | *'with BFD'*)
  LDFLAGS="$LDFLAGS -Wl,-Bsymbolic-functions"
  AC_MSG_RESULT([yes])
  ;;
*)                                                                              
  AC_MSG_RESULT([no])
  ;;
esac

dnl advanced compiler tweaking
CPPFLAGS="$CPPFLAGS -fstrict-aliasing -fmessage-length=0"


# program paths
AC_ARG_WITH(mount-prog, 
	AC_HELP_STRING([--with-mount-prog=PATH], [Path to mount program (default: /bin/mount)]),
	AC_DEFINE_UNQUOTED(MOUNTPROG, "$withval", [path to mount]),
	AC_DEFINE_UNQUOTED(MOUNTPROG, "/bin/mount", [path to mount]))
mountprog="$withval"
AC_ARG_WITH(umount-prog, 
	AC_HELP_STRING([--with-umount-prog=PATH], [Path to umount program (default: /bin/umount)]),
	AC_DEFINE_UNQUOTED(UMOUNTPROG, "$withval", [path to umount]),
	AC_DEFINE_UNQUOTED(UMOUNTPROG, "/bin/umount", [path to umount]))
umountprog="$withval"
AC_ARG_WITH(losetup-prog, 
	AC_HELP_STRING([--with-losetup-prog=PATH], [Path to losetup program (default: /sbin/losetup)]),
	AC_DEFINE_UNQUOTED(LOSETUPPROG, "$withval", [path to losetup]),
	AC_DEFINE_UNQUOTED(LOSETUPPROG, "/sbin/losetup", [path to losetup]))
losetupprog="$withval"
AC_ARG_WITH(setfacl-prog, 
	AC_HELP_STRING([--with-setfacl-prog=PATH], [Path to setfacl program (default: /usr/bin/setfacl)]),
	AC_DEFINE_UNQUOTED(SETFACLPROG, "$withval", [path to setfacl]),
	AC_DEFINE_UNQUOTED(SETFACLPROG, "/usr/bin/setfacl", [path to setfacl]))
setfaclprog="$withval"


# options
AC_ARG_ENABLE(
    [systemd],
    AS_HELP_STRING([--disable-systemd],
                   [disable installation of devmon daemon files for systemd (default: enable)]),
		   use_systemd=$enableval, use_systemd="yes")
if test x"$use_systemd" = x"yes"; then
    AC_DEFINE([ADD_SYSTEMD], [1], [Copy devmon daemon systemd files])
fi
AM_CONDITIONAL(ADD_SYSTEMD, test "$use_systemd" = "yes")


# check for libs    gobject-2.0 gmodule-2.0 gthread-2.0
dnl Check for glib 2.0
PKG_CHECK_MODULES(GLIB, [glib-2.0], , [
	AC_MSG_RESULT(no)
	AC_MSG_ERROR([

You must install GLib 2.0 development headers (eg libglib2.0-dev) to build.

If you have these installed already you may need to install pkg-config so
I can find them.
])])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

dnl Check udev support
PKG_CHECK_MODULES(LIBUDEV, [libudev >= 143], , [
	AC_MSG_RESULT(no)
	AC_MSG_ERROR([

You must install libudev development headers (eg libudev-dev) to build.

If you have these installed already you may need to install pkg-config so
I can find them.
])])
AC_SUBST(LIBUDEV_CFLAGS)
AC_SUBST(LIBUDEV_LIBS)


#dnl Large file support
CPPFLAGS="$CPPFLAGS -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64"
AC_DEFINE(_LARGEFILE64_SOURCE, 1, [Whether to enable large file support])
AC_DEFINE(_FILE_OFFSET_BITS, 64, [File offset bits])
AC_DEFINE(_LARGEFILE_SOURCE, 1, [Whether to enable large file support])


GETTEXT_PACKAGE="udevil"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

dnl Add the languages which your application supports here.
#ALL_LINGUAS="af ar da ca cs de es et eu fa fi fr gl he hr hu id it ja ko lt ml ms nb nl nn pl ps pt pt_BR ru sk sl sv tr uk ur ur_PK vi zh_CN zh_TW"
ALL_LINGUAS="ru"
AM_GLIB_GNU_GETTEXT


AC_OUTPUT([
Makefile
po/Makefile.in
man/Makefile
etc/Makefile
src/Makefile
])


echo
echo udevil....................................... : Version $VERSION
echo
echo Linux device support......................... : libudev
if test x"$mountprog" != x""; then
echo mount program................................ : $mountprog
else
echo mount program................................ : /bin/mount
fi
if test x"$umountprog" != x""; then
echo umount program............................... : $umountprog
else
echo umount program............................... : /bin/umount
fi
if test x"$losetupprog" != x""; then
echo losetup program.............................. : $losetupprog
else
echo losetup program.............................. : /sbin/losetup
fi
if test x"$setfaclprog" != x""; then
echo setfacl program.............................. : $setfaclprog
else
echo setfacl program.............................. : /usr/bin/setfacl
fi
echo Install devmon systemd service files ?....... : $use_systemd
echo Installation Prefix.......................... : $prefix
echo "      The binary will be installed as $prefix/bin/udevil"
echo sysconfdir................................... : $sysconfdir
echo "      The configuration will be installed in $sysconfdir/udevil"
echo
echo 'Homepage: http://ignorantguru.github.io/udevil/'
echo

