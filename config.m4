PHP_ARG_ENABLE(mulq, whether to enable mulq support,
Make sure that the comment is aligned:
[  --enable-mulq           Enable mulq support])

PHP_ARG_WITH(mysql-dir, for MySQL Connector,
[  --with-mysql-dir[=DIR]   MySQL Connector Path], yes)

if test "$PHP_MULQ" != "no"; then
    if test "$PHP_MYSQL_DIR" != "no" && test "$PHP_MYSQL_DIR" != "yes"; then
        AC_MSG_RESULT([$PHP_MYSQL_DIR])
        PHP_ADD_INCLUDE($PHP_MYSQL_DIR/include)
        PHP_ADD_LIBRARY_WITH_PATH(mysqlclient_r, $PHP_MYSQL_DIR/$PHP_LIBDIR, MULQ_SHARED_LIBADD)
    else
        AC_MSG_ERROR([mulq requires mysql connector])
    fi
    PHP_ADD_LIBRARY_WITH_PATH(pthread, , MULQ_SHARED_LIBADD)
    PHP_SUBST(MULQ_SHARED_LIBADD)

    PHP_NEW_EXTENSION(mulq, php_mulq.c mulq.c, $ext_shared)
fi