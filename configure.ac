AC_PREREQ([2.69])

AC_INIT([glusterfs-coreutils],
        m4_esyscmd(echo -n `git describe --always --tags`),
        [gluster-users@gluster.org],,
        [https://github.com/gluster/gluster-coreutils.git])

AC_SUBST([PACKAGE_RELEASE], 1)

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([glusterfs-coreutils.spec Makefile lib/Makefile src/Makefile man/Makefile])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.15 foreign -Wall -Werror tar-ustar])
AM_MAINTAINER_MODE

AC_DEFINE([COPYRIGHT], ["Copyright (C) 2015 Facebook, Inc."], [Copyright notice])
AC_DEFINE_UNQUOTED([LICENSE], ["License GPLv3: GNU GPL version 3 " \
           "<http://www.gnu.org/licenses/gpl-3.0.en.html>.\nThis is free software: you " \
           "are free to change and redistribute it.\nThere is NO WARRANTY, " \
           "to the extent permitted by law.\n"], [License text])

# Configure libtool
LT_INIT

# Checks
AC_PROG_CC
AC_PROG_CC_C99
gl_EARLY
gl_INIT
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_EGREP

# Dependencies
AC_CHECK_HEADER([glusterfs/api/glfs.h],,[AC_MSG_ERROR([cannot find glusterfs api headers])])
PKG_CHECK_MODULES([GLFS], [glusterfs-api >= 3])
PKG_CHECK_MODULES([TIRPC], [libtirpc >= 0.2.1])

AC_CHECK_PROG([HAVE_HELP2MAN],[help2man],[yes],[no])
AM_CONDITIONAL([HAVE_HELP2MAN], [test "x$HAVE_HELP2MAN" = xyes])
AM_COND_IF([HAVE_HELP2MAN],,[AC_MSG_ERROR([required program 'help2man' not found.])])

AC_SUBST([MORE_CFLAGS])
AC_SUBST([MORE_CPPFLAGS])
AC_SUBST([MORE_LDFLAGS])

AC_OUTPUT

cat <<EOF

------------------ Summary ------------------
 $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  C Compiler.....: $CC $CFLAGS $CPPFLAGS
  Linker.........: $LD $MORE_LDFLAGS $LDFLAGS $LIBS
---------------------------------------------

Check the above options and compile with:
 ${MAKE-make}

EOF
