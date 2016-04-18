dnl Process this file with autoconf to produce a configure script.

AC_INIT
AM_INIT_AUTOMAKE(wlziipsrv,1.1.5)

AC_PREFIX_DEFAULT( `pwd` )


dnl Checks for programs, but watch out for automake lex/yacc crazyness.
dnl I've named the variables MYLEX and MYYACC to try and avoid these
dnl problems, but some still persist.  Don't use AC_PROG_YACC it causes
dnl yywrap to be used and yywrap doesn't support the command line options
dnl needed.
MYLEX=flex
YACC=bison
NIFTI_DIR="no"
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_CHECK_PROG(MYLEX,flex,flex,noflex)
AC_CHECK_PROG(MYYACC,bison,bison,nobison)
if test "$MYYACC" != "bison"
then
  AC_MSG_ERROR("wlziipsrv requires the GNU bison to build");
fi
if test "$MYLEX" != "flex"
then
  AC_MSG_ERROR("wlziipsrv requires GNU flex to build");
else
  MYLEX_LIBS=-lfl
  AC_SUBST(MYLEX_LIBS)
fi

dnl   AC_MSG_ERROR(unable to find libjpeg),

dnl Checks for header files.
AC_CHECK_HEADERS(glob.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(math.h)
AC_CHECK_LIB(m, pow)

dnl For our windows build
AC_CHECK_HEADERS(windows.h)


# we want largefile support, if possible
AC_SYS_LARGEFILE


dnl	************************************************************ 


dnl Check for compile options.
dnl AC_ARG_ENABLE(debug,
dnl     [  --enable-debug          compile with debugging on],
dnl     , )
dnl AC_DEFINE(DEBUG))


dnl	************************************************************ 


dnl 	Check for a user specified location for libjpeg

dnl AC_ARG_WITH(libjpeg-incl,
dnl   [  --with-libjpeg-incl=DIR     location of the jpeg library include files],
dnl   jpeg_incl=-I$withval)

dnl AC_ARG_WITH(libjpeg-lib,
dnl   [  --with-libjpeg-lib=DIR     location of the jpeg library files],
dnl   jpeg_lib=-L$withval)

dnl AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
dnl   LIBJPEG_INCL=$jpeg_incl; LIBJPEG_LIB="$jpeg_lib -ljpeg",
dnl   AC_MSG_ERROR(unable to find libjpeg),
dnl   $jpeg_lib)

FIND_JPEG(,[AC_MSG_ERROR([libjpeg not found])])


dnl	************************************************************ 

dnl	Check for a standard libz

AC_CHECK_LIB(z, gzopen)

dnl	************************************************************ 


dnl 	Check for user specified location for libtiff

dnl AC_ARG_WITH(libtiff-incl,
dnl   [  --with-libtiff-incl=DIR     location of the tiff library include files],
dnl   tiff_incl=-I$withval)

dnl AC_ARG_WITH(libtiff-lib,
dnl   [  --with-libtiff-lib=DIR     location of the tiff library files],
dnl   tiff_lib=-L$withval)

dnl AC_CHECK_LIB(tiff, TIFFReadScanline,
dnl   LIBTIFF_INCL=$tiff_incl; LIBTIFF_LIB="$tiff_lib -ltiff -ljpeg -lm",
dnl   AC_MSG_ERROR(unable to find libtiff),
dnl   $tiff_lib -ljpeg -lm)

FIND_TIFF(,[AC_MSG_ERROR([libtiff not found])])

dnl     ************************************************************

dnl     Check for little cms library

AC_CHECK_HEADERS(lcms/lcms.h, AC_CHECK_LIB(lcms, cmsOpenProfileFromMem, LIBS="${LIBS} -llcms";AC_DEFINE(LCMS) ) )



dnl	************************************************************ 
dnl 	Check for user specified location for libpng

dnl AC_ARG_WITH(libpng-incl,
dnl   [  --with-libpng-incl=DIR     location of the png library include files],
dnl   png_incl=-I$withval)

dnl AC_ARG_WITH(libpng-lib,
dnl   [  --with-libpng-lib=DIR     location of the png library files],
dnl   png_lib=-L$withval)

dnl AC_CHECK_LIB(png, png_write_png,
dnl   LIBPNG_INCL=$png_incl; LIBTPNG_LIB="$png_lib -lpng",
dnl   AC_MSG_ERROR(unable to find libpng),
dnl   $png_lib -lpng)

FIND_PNG(,[AC_MSG_ERROR([libpng not found])])

dnl	************************************************************ 
dnl 	Check for user specified locations for fast cgi library


AC_ARG_WITH( fcgi-incl,
  [  --with-fcgi-incl=DIR          location of the fcgi include files],
fcgi_include_path=-I$withval)

AC_ARG_WITH( fcgi-lib,
  [  --with-fcgi-lib=DIR           location of the fcgi libraries],
  fcgi_lib_path=-L$withval;
  fcgi_lib_path_only=$withval)

AC_ARG_ENABLE(static-fcgi,
 [  --enable-static-fcgi    enable staticaly linked libfcgi],
	echo "Using static fcgi library";
	LIBFCGI_INCLUDES="$fcgi_include_path";
	LIBFCGI_LIBS="$fcgi_lib_path_only/libfcgi.a",
	AC_CHECK_LIB( fcgi, main,echo "fcgi found";
		LIBFCGI_INCLUDES="$fcgi_include_path";
		LIBFCGI_LDFLAGS="$fcgi_lib_path";
		LIBFCGI_LIBS="-lfcgi",
	echo "Using fcgi library from $fcgi_lib_path and includes from";
	echo "$fcgi_include_path .";
	LIBFCGI_LDFLAGS="$fcgi_lib_path";
	LIBFCGI_INCLUDES="$fcgi_include_path";
	LIBFCGI_LIBS="-lfcgi" ))

AC_SUBST(LIBFCGI_INCLUDES)
AC_SUBST(LIBFCGI_LDFLAGS)
AC_SUBST(LIBFCGI_LIBS)

dnl	************************************************************ 
dnl 	Check for user specified locations for the NIfTI library

nifti_include_path=""
nifti_lib_path=""

AC_ARG_WITH( nifti-incl,
   [  --with-nifti-incl=DIR         location of the NIfTI include files],
   nifti_include_path=$withval)


AC_ARG_WITH( nifti-lib,
   [  --with-nifti-lib=DIR          location of the NIfTI libraries],
   nifti_lib_path=$withval)

if test x$nifti_lib_path != x -a x$nifti_lib_path != x
then
  LIBNIFTI_INCLUDES="-I$nifti_include_path";
  LIBNIFTI_LDFLAGS="-L$nifti_lib_path";
  LIBNIFTI_LIBS="-lniftiio -lznz";
  AC_DEFINE(HAVE_NIFTI, 1, [Define if NIfTI library used.])

  AC_SUBST(LIBNIFTI_INCLUDES)
  AC_SUBST(LIBNIFTI_LDFLAGS)
  AC_SUBST(LIBNIFTI_LIBS)
fi

dnl	************************************************************ 
dnl 	Check for user specified locations for log4cpp library


AC_ARG_WITH( log4cpp-incl,
  [  --with-log4cpp-incl=DIR       location of the log4cpp include files],
log4cpp_include_path=-I$withval)

AC_ARG_WITH( log4cpp-lib,
  [  --with-log4cpp-lib=DIR        location of the log4cpp libraries],
  log4cpp_lib_path=-L$withval;
  log4cpp_lib_path_only=$withval)

if test x$log4cpp_lib_path != x -a x$log4cpp_lib_path != x
then
  LIBLOG4CPP_INCLUDES="-I$log4cpp_include_path";
  LIBLOG4CPP_LDFLAGS="-L$log4cpp_lib_path";
  LIBLOG4CPP_LIBS="-llog4cpp";
  AC_DEFINE(HAVE_LOG4CPP, 1, [Define if log4cpp library used.])

  AC_SUBST(LIBLOG4CPP_INCLUDES)
  AC_SUBST(LIBLOG4CPP_LDFLAGS)
  AC_SUBST(LIBLOG4CPP_LIBS)
fi

AC_SUBST(LIBLOG4CPP_INCLUDES)
AC_SUBST(LIBLOG4CPP_LDFLAGS)
AC_SUBST(LIBLOG4CPP_LIBS)

dnl	************************************************************ 
dnl 	Check for user specified locations for Woolz libraries

AC_ARG_WITH( wlz-incl,
   [  --with-wlz-incl=DIR           location of the Woolz include files],
   wlz_include_path=$withval)


AC_ARG_WITH( wlz-lib,
   [  --with-wlz-lib=DIR            location of the Woolz libraries],
   wlz_lib_path=$withval)

LIBWLZ_INCLUDES="-I$wlz_include_path";
LIBWLZ_LDFLAGS="-L$wlz_lib_path";
LIBWLZ_LIBS="-lWlzExtFF -lhguDlpList -lWlz -lbibfile -lAlg -lAlc "

AC_SUBST(LIBWLZ_INCLUDES)
AC_SUBST(LIBWLZ_LDFLAGS)
AC_SUBST(LIBWLZ_LIBS)


dnl	************************************************************ 
dnl 	Check for openmp used

AC_ARG_ENABLE(openmp,
    [  --enable-openmp         build and link with openmp] )

if test "$enable_openmp" = "yes"
then
	openmp=true
	# GNU CC
	if test "$CC" = "gcc"
	then
	  CFLAGS="${CFLAGS} -fopenmp"
	  CXXFLAGS="${CXXFLAGS} -fopenmp"
	fi
	# Intel CC
	if test "$CC" = "icc"
	then
	  CFLAGS="${CFLAGS} -openmp"
	  CXXFLAGS="${CXXFLAGS} -openmp"
	fi

fi

AM_CONDITIONAL( ENABLE_LTO, test x$lto = xtrue )

dnl	************************************************************ 
dnl 	Check for link time optimisation used

AC_ARG_ENABLE(lto,
    [  --enable-lto		build with link time optimisation] )

if test "$enable_lto" = "yes"
then
	lto=true
	# GNU CC
	if test "$CC" = "gcc"
	then
	  CFLAGS="${CFLAGS} -flto"
	  CXXFLAGS="${CXXFLAGS} -flto"
	fi
	# Intel CC
	if test "$CC" = "icc"
	then
	  CFLAGS="${CFLAGS} -ipo"
	  CXXFLAGS="${CXXFLAGS} -ipo"
	fi

fi

AM_CONDITIONAL( ENABLE_LTO, test x$lto = xtrue )

dnl	************************************************************ 
dnl 	Check for libdl for dynamic library loading

AC_ARG_ENABLE(modules,
    [  --enable-modules        enable dynamic module loading] )

if test "$enable_modules" = "yes"; then

	modules=true
	AC_CHECK_HEADERS(dlfcn.h)
	AC_CHECK_LIB(dl, dlopen, \
		DL_LIBS="-ldl"; AC_DEFINE(ENABLE_DL), \
		AC_MSG_ERROR(unable to find libdl) )

else
	modules=false
	AC_MSG_WARN( optional dynamic module loading not enabled)

fi

AM_CONDITIONAL( ENABLE_MODULES, test x$modules = xtrue )
AC_SUBST(DL_LIBS)


dnl	************************************************************




INCLUDES="$INCLUDES -I."
AC_SUBST(INCLUDES)

AC_SUBST(LIBS)



dnl	************************************************************
dnl     FCGI library configure
dnl	************************************************************


AC_LANG([C++])

dnl autoconf defaults CXX to 'g++', so its unclear whether it exists/works
AC_MSG_CHECKING([whether $CXX works])
AC_TRY_COMPILE([#include <iostream>], 
               [std::cout << "ok";], 
               [AC_MSG_RESULT(yes)
                LIBFCGIXX=libfcgi++.la
                ECHO_CPP=echo-cpp${EXEEXT}
                AC_MSG_CHECKING([whether cin has a streambuf assignment operator])
                AC_TRY_COMPILE([#include <iostream>], 
                               [cin = static_cast<streambuf *>(0);], 
                               [AC_MSG_RESULT(yes)
                                AC_DEFINE([HAVE_IOSTREAM_WITHASSIGN_STREAMBUF],  [1], 
                                          [Define if cin/cout/cerr has a streambuf assignment operator])],
                               [AC_MSG_RESULT(no)]) 
                AC_MSG_CHECKING([whether char_type is defined in the context of streambuf])
                AC_TRY_COMPILE([#include <iostream>],
                               [class fcgi_streambuf : public std::streambuf { char_type ct; }],
                               [AC_MSG_RESULT(yes)
                                AC_DEFINE([HAVE_STREAMBUF_CHAR_TYPE], [1], 
                                          [Define if char_type is defined in the context of streambuf])],
                               [AC_MSG_RESULT(no)])],
               [AC_MSG_RESULT(no)])
AC_SUBST(LIBFCGIXX)
AC_SUBST(ECHO_CPP)

AC_LANG([C])

AC_CHECK_LIB([nsl],       [gethostbyname])
AC_CHECK_LIB([socket],    [socket]) 


ACX_PTHREAD([THREADED=threaded${EXEEXT}])
AC_SUBST([THREADED])


FCGI_COMMON_CHECKS()

AC_REPLACE_FUNCS([strerror])

AC_C_INLINE

SYSTEM=unix
AC_SUBST([SYSTEM])

AC_PROG_CC_WARNINGS



AC_PROG_MAKE_SET
AC_OUTPUT(Makefile \
	src/Makefile )

AC_MSG_RESULT([
The Woolz IIP server has been configured on ${host} with

  Source directory:                 ${srcdir}
  Installation directory:           ${prefix}
  Target CPU:                       ${target_cpu}
  C compiler:                       ${CC}
  Linker:                           ${LD}
  Archiver:                         ${AR}
  CFLAGS:                           ${CFLAGS}
  CXXFLAGS:                         ${CXXFLAGS}
  LDFLAGS:                          ${LDFLAGS}

To build:                          make
])
