dnl $Id$
dnl config.m4 for extension clamav

PHP_ARG_WITH(clamav, for clamav support,
[  --with-clamav             Include clamav support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(clamav, whether to enable clamav support,
dnl Make sure that the comment is aligned:
dnl [  --enable-clamav           Enable clamav support])

if test "$PHP_CLAMAV" != "no"; then

  dnl # --with-clamav -> check with-path
  SEARCH_PATH="/usr/local /usr"     # you might want to change this
  SEARCH_FOR="/include/clamav.h"  # you most likely want to change this
  if test -r $PHP_CLAMAV/$SEARCH_FOR; then # path given as parameter
    CLAMAV_DIR=$PHP_CLAMAV
  else # search default path list
    AC_MSG_CHECKING([for clamav files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        CLAMAV_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$CLAMAV_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the clamav distribution])
  fi

  dnl # --with-clamav -> add include path
  PHP_ADD_INCLUDE($CLAMAV_DIR/include)

  dnl # --with-clamav -> check for lib and symbol presence
  LIBNAME=clamav # you may want to change this
  LIBSYMBOL=cl_retver # you most likely want to change this 

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $CLAMAV_DIR/lib, CLAMAV_SHARED_LIBADD)
    AC_DEFINE(HAVE_CLAMAVLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong clamav lib version or lib not found])
  ],[
    -L$CLAMAV_DIR/lib -lm
  ])

  PHP_SUBST(CLAMAV_SHARED_LIBADD)

  PHP_NEW_EXTENSION(clamav, clamav.c, $ext_shared)
fi
