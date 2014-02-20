AC_INIT([vxi11], [0.0.0])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
LT_INIT
AC_LANG_PUSH([C++])

ARCH=`uname -m`

AC_PROG_CXX

# ------------------------------------------------------------------------------
# DOXYGEN
# ------------------------------------------------------------------------------

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

AM_CONDITIONAL([HAVE_DOXYGEN],[test -n "$DOXYGEN"])
#AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])
if test -n "$DOXYGEN"; then
    AC_CONFIG_FILES([doc/Doxyfile])
fi

# ------------------------------------------------------------------------------
# EPILOG
# ------------------------------------------------------------------------------
    
dnl: this is a comment
dnl: ${CXXFLAGS=-g}

AC_CONFIG_FILES([Makefile doc/Makefile lib/Makefile src/Makefile])
AC_OUTPUT


