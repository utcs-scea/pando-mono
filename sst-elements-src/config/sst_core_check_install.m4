
AC_DEFUN([SST_CORE_CHECK_INSTALL], [
	AC_ARG_WITH([sst-core],
	  [AS_HELP_STRING([--with-sst-core=@<:@=DIR@:>@],
	    [Use SST Discrete Event Core installed in DIR])],
	    [with_sst_core=$withval/bin],
	    [with_sst_core=$PATH])

  SST_CONFIG_TOOL=""
  SST_REGISTER_TOOL=""

  AS_IF( [test x"$with_sst_core" = "x/bin"],
	 [AC_MSG_ERROR([User undefined path while using --with-sst-core], [1])],
	 [AC_MSG_NOTICE([--with-sst-core check = good 1])] )

  AS_IF( [test x"$with_sst_core" = "xyes/bin"],
	 [AC_MSG_ERROR([User undefined path while using --with-sst-core], [1])],
	 [AC_MSG_NOTICE([--with-sst-core check = good 2])] )

  AC_PATH_PROG([SST_CONFIG_TOOL], [sst-config], [], [$with_sst_core])

  AC_MSG_CHECKING([for sst-config tool available])
  AS_IF([test -x "$SST_CONFIG_TOOL"],
	[AC_MSG_RESULT([found $SST_CONFIG_TOOL])],
	[AC_MSG_ERROR([Unable to find sst-config in $with_sst_core], [1])])

  AC_PATH_PROG([SST_REGISTER_TOOL], [sst-register], [], [$with_sst_core])

  AC_MSG_CHECKING([for sst-register tool available])
  AS_IF([test -x "$SST_REGISTER_TOOL"],
	[AC_MSG_RESULT([found $SST_REGISTER_TOOL])],
	[AC_MSG_ERROR([Unable to find sst-register in $with_sst_core], [1])])

  SST_CC=`$SST_CONFIG_TOOL --CC`
  SST_CXX=`$SST_CONFIG_TOOL --CXX`
  SST_MPICC=`$SST_CONFIG_TOOL --MPICC`
  SST_MPICXX=`$SST_CONFIG_TOOL --MPICXX`
  SST_PREFIX=`$SST_CONFIG_TOOL --prefix`
  SST_CPPFLAGS=`$SST_CONFIG_TOOL --CPPFLAGS`
  SST_CFLAGS=`$SST_CONFIG_TOOL --CFLAGS`
  SST_CXXFLAGS=`$SST_CONFIG_TOOL --CXXFLAGS`
  SST_LDFLAGS=`$SST_CONFIG_TOOL --LDFLAGS`
  SST_LIBS=`$SST_CONFIG_TOOL --LIBS`
  SST_PREVIEW_BUILD=`$SST_CONFIG_TOOL --PREVIEW_BUILD`

  CC="$SST_CC"
  CPP="$SST_CC"
  CXX="$SST_CXX"
  MPICC="$SST_MPICC"
  MPICXX="$SST_MPICXX"

  PYTHON_CPPFLAGS=`$SST_CONFIG_TOOL --PYTHON_CPPFLAGS`
  PYTHON_LDFLAGS=`$SST_CONFIG_TOOL --PYTHON_LDFLAGS`

  TEMP_CPPFLAGS="$SST_CPPFLAGS $PYTHON_CPPFLAGS -I$SST_PREFIX/include/sst/core -I$SST_PREFIX/include"
  TEMP_CFLAGS="$SST_CFLAGS"
  TEMP_CXXFLAGS="$SST_CXXFLAGS"
  TEMP_LDFLAGS="$SST_LDFLAGS $PYTHON_LDFLAGS"
  LIBS="$SST_LIBS $LIBS"

  AC_SUBST([AM_CPPFLAGS], [$TEMP_CPPFLAGS])
  AC_SUBST([AM_CFLAGS], [$TEMP_CFLAGS])
  AC_SUBST([AM_CXXFLAGS], [$TEMP_CXXFLAGS])
  AC_SUBST([AM_LDFLAGS], [$TEMP_LDFLAGS])

  AC_SUBST([SST_CONFIG_TOOL])
  AC_SUBST([SST_REGISTER_TOOL])
  AC_SUBST([SST_PREFIX])

  AM_CONDITIONAL([SST_ENABLE_PREVIEW_BUILD], [test "x$SST_PREVIEW_BUILD" = "xyes"])

])
