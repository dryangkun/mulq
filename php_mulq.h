/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header 297205 2010-03-30 21:09:07Z johannes $ */

#ifndef PHP_MULQ_H
#define PHP_MULQ_H

extern zend_module_entry mulq_module_entry;
#define phpext_mulq_ptr &mulq_module_entry

#ifdef PHP_WIN32
#define PHP_MULQ_API __declspec(dllexport)
#else
#define PHP_MULQ_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(mulq);
PHP_MSHUTDOWN_FUNCTION(mulq);
PHP_RINIT_FUNCTION(mulq);
PHP_RSHUTDOWN_FUNCTION(mulq);
PHP_MINFO_FUNCTION(mulq);

PHP_METHOD(MulqServer, host);

PHP_METHOD(MulqUnit, server);
PHP_METHOD(MulqUnit, bind);

PHP_METHOD(MulqSet, attach);
PHP_METHOD(MulqSet, bind);

PHP_METHOD(Mulq, attach);
PHP_METHOD(Mulq, run);

#ifdef ZTS
#define MULQ_G(v) TSRMG(mulq_globals_id, zend_mulq_globals *, v)
#else
#define MULQ_G(v) (mulq_globals.v)
#endif

#endif	/* PHP_MULQ_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
