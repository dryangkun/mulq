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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_mulq.h"
#include "mulq.h"

static zend_class_entry *mulq_ce = NULL;
static zend_class_entry *mulqset_ce = NULL;
static zend_class_entry *mulqunit_ce = NULL;
static zend_class_entry *mulqserver_ce = NULL;

static zend_object_handlers mulq_object_handlers;
static zend_object_handlers mulqset_object_handlers;
static zend_object_handlers mulqunit_object_handlers;
static zend_object_handlers mulqserver_object_handlers;

ZEND_BEGIN_ARG_INFO(mulq_attach_arg, 0)
ZEND_ARG_INFO(0, unit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(mulqset_attach_arg, 0)
ZEND_ARG_OBJ_INFO(0, unit, MulqUnit, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(mulqset_bind_arg, 0)
ZEND_ARG_ARRAY_INFO(1, data, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(mulqunit_server_arg, 0)
ZEND_ARG_OBJ_INFO(0, server, MulqServer, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO(mulqunit_bind_arg, 0)
ZEND_ARG_ARRAY_INFO(0, sql, 0)
ZEND_ARG_INFO(0, callback)
ZEND_ARG_ARRAY_INFO(1, data, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(mulqserver_host_arg, 0)
ZEND_ARG_ARRAY_INFO(0, host, 0)
ZEND_END_ARG_INFO()

static zend_function_entry mulq_methods[] = {
    PHP_ME(Mulq, attach, mulq_attach_arg, ZEND_ACC_PUBLIC)
    PHP_ME(Mulq, run, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static zend_function_entry mulqset_methods[] = {
    PHP_ME(MulqSet, attach, mulqset_attach_arg, ZEND_ACC_PUBLIC)
    PHP_ME(MulqSet, bind, mulqset_bind_arg, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

static zend_function_entry mulqunit_methods[] = {
    PHP_ME(MulqUnit, server, mulqunit_server_arg, ZEND_ACC_PUBLIC)
    PHP_ME(MulqUnit, bind, mulqunit_bind_arg, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

static zend_function_entry mulqserver_methods[] = {
    PHP_ME(MulqServer, host, mulqserver_host_arg, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

/* {{{ Mulq object destruct
 */
static void php_mulq_destroy_storage(Mulq *mulq TSRMLS_DC)
{
    zend_object_std_dtor(&mulq->zobject TSRMLS_CC);
    mulq_destroy(&mulq);
}
/* }}} */

/* {{{ Mulq object construct
 */
zend_object_value php_mulq_init(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
	Mulq *mulq;
	zval *tmp;

	mulq_init(&mulq);
	zend_object_std_init(&mulq->zobject, ce TSRMLS_CC);
#if PHP_VERSION_ID >= 50400
	object_properties_init(&mulq->zobject, ce);
#else
	zend_hash_copy(mulq->zobject.properties, &ce->default_properties, (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#endif

	retval.handle = zend_objects_store_put(mulq, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_mulq_destroy_storage, NULL TSRMLS_CC);
	retval.handlers = &mulq_object_handlers;

	return retval;
}
/* }}} */

/* {{{ MulqSet object destruct
 */
static void php_mulqset_destroy_storage(MulqSet *set TSRMLS_DC)
{
    zend_object_std_dtor(&set->zobject TSRMLS_CC);
    mulq_set_destroy(&set);
}
/* }}} */

/* {{{ MulqSet object construct
 */
zend_object_value php_mulqset_init(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
	MulqSet *set;
	zval *tmp;
    
	mulq_set_init(&set);
	zend_object_std_init(&set->zobject, ce TSRMLS_CC);
    
#if PHP_VERSION_ID >= 50400
	object_properties_init(&set->zobject, ce);
#else
	zend_hash_copy(set->zobject.properties, &ce->default_properties, (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#endif

	retval.handle = zend_objects_store_put(set, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_mulqset_destroy_storage, NULL TSRMLS_CC);
	retval.handlers = &mulqset_object_handlers;
    
	return retval;
}
/* }}} */

/* {{{ MulqUnit object destruct
 */
static void php_mulqunit_destroy_storage(MulqUnit *unit TSRMLS_DC)
{
    zend_object_std_dtor(&unit->zobject TSRMLS_CC);
    mulq_unit_destroy(&unit);
}
/* }}} */

/* {{{ MulqUnit object construct
 */
zend_object_value php_mulqunit_init(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
	MulqUnit *unit;
	zval *tmp;
    
    mulq_unit_init(&unit);
	zend_object_std_init(&unit->zobject, ce TSRMLS_CC);
#if PHP_VERSION_ID >= 50400
    object_properties_init(&unit->zobject, ce);
#else
	zend_hash_copy(unit->zobject.properties, &ce->default_properties, (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#endif

	retval.handle = zend_objects_store_put(unit, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_mulqunit_destroy_storage, NULL TSRMLS_CC);
	retval.handlers = &mulqunit_object_handlers;

	return retval;
}
/* }}} */

/* {{{ MulqServer object destruct
 */
static void php_mulqserver_destroy_storage(MulqServer *server TSRMLS_DC)
{
    zend_object_std_dtor(&server->zobject TSRMLS_CC);
    mulq_server_destroy(&server);
}
/* }}} */

/* {{{ MulqServer object construct
 */
zend_object_value php_mulqserver_init(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
	MulqServer *server;
	zval *tmp;

	mulq_server_init(&server);
	zend_object_std_init(&server->zobject, ce TSRMLS_CC);
#if PHP_VERSION_ID >= 50400
	object_properties_init(&server->zobject, ce);
#else
	zend_hash_copy(server->zobject.properties, &ce->default_properties, (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#endif

	retval.handle = zend_objects_store_put(server, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_mulqserver_destroy_storage, NULL TSRMLS_CC);
	retval.handlers = &mulqserver_object_handlers;

	return retval;
}
/* }}} */

/* {{{ php mulq unit callback
 */
void php_mulqunit_callback(void *data, const char **row, ulong *rlength, const char **field, uint *flength, uint fnum, void *arg TSRMLS_DC)
{
    MulqUnit *unit;
    HashPosition pos;
    zend_fcall_info fci;
    
    zval *zdata, *zv, **item;
    zval **args[2];
    zval *retval_ptr;
    int i;
    
    zdata = (zval *)data;
    memcpy(&unit, &arg, sizeof(MulqUnit *));
    
    ALLOC_INIT_ZVAL(zv);
    array_init(zv);
    
    for (i = 0; i < fnum; ++i) {
        zval *tmp;
        MAKE_STD_ZVAL(tmp);
        ZVAL_STRINGL(tmp, (char *)row[i], rlength[i], 1);
        zval_add_ref(&tmp);
        
        zend_hash_update(Z_ARRVAL_P(zv), (char *)field[i], flength[i] + 1, (void *)&tmp, sizeof(zval *), NULL);
    }
    
    args[0] = &zdata;
    args[1] = &zv;
    
    fci.size = sizeof(fci);
	fci.function_table = EG(function_table);
	fci.function_name = &unit->func_name;
	fci.symbol_table = NULL;
	fci.object_pp = NULL;
	fci.retval_ptr_ptr = &retval_ptr;
    fci.param_count = 2;
	fci.params = args;
	fci.no_separation = 0;
    
    if (zend_call_function(&fci, NULL TSRMLS_CC) != SUCCESS) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The callback fail");
    } else {
        zval_ptr_dtor(&retval_ptr);
    }
   
    for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(zv), &pos);
         zend_hash_get_current_data_ex(Z_ARRVAL_P(zv), (void **)&item, &pos) == SUCCESS;
         zend_hash_move_forward_ex(Z_ARRVAL_P(zv), &pos)
    ) {
        zval_ptr_dtor(item);
    }
    zval_ptr_dtor(&zv);
}
/* }}} */

/* {{{ mulq_module_entry
 */
zend_module_entry mulq_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"mulq",
	NULL,
	PHP_MINIT(mulq),
	PHP_MSHUTDOWN(mulq),
	PHP_RINIT(mulq),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(mulq),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(mulq),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MULQ
ZEND_GET_MODULE(mulq)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(mulq)
{
	zend_class_entry mulq;
    zend_class_entry mulqset;
    zend_class_entry mulqunit;
    zend_class_entry mulqserver;
    
    memcpy(&mulq_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mulq_object_handlers.clone_obj = NULL;
    
    memcpy(&mulqset_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    mulqset_object_handlers.clone_obj = NULL;
    
    memcpy(&mulqunit_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    mulqunit_object_handlers.clone_obj = NULL;
    
    memcpy(&mulqserver_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    mulqserver_object_handlers.clone_obj = NULL;
    
    INIT_CLASS_ENTRY(mulq, "Mulq", mulq_methods);
    INIT_CLASS_ENTRY(mulqset, "MulqSet", mulqset_methods);
    INIT_CLASS_ENTRY(mulqunit, "MulqUnit", mulqunit_methods);
    INIT_CLASS_ENTRY(mulqserver, "MulqServer", mulqserver_methods);
    
    mulq_ce = zend_register_internal_class_ex(&mulq, NULL, NULL TSRMLS_CC);
    mulqset_ce = zend_register_internal_class_ex(&mulqset, NULL, NULL TSRMLS_CC);
    mulqunit_ce = zend_register_internal_class_ex(&mulqunit, NULL, NULL TSRMLS_CC);
    mulqserver_ce = zend_register_internal_class_ex(&mulqserver, NULL, NULL TSRMLS_CC);
    
    mulq_ce->create_object = php_mulq_init;
    mulqset_ce->create_object = php_mulqset_init;
    mulqunit_ce->create_object = php_mulqunit_init;
    mulqserver_ce->create_object = php_mulqserver_init;
    
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(mulq)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(mulq)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(mulq)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(mulq)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "mulq support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ Mulq attach unit or set
 */
PHP_METHOD(Mulq, attach)
{
    zval *zobj, *object = getThis();
    zend_bool is_set, is_unit;
    Mulq *mulq;
    MulqSet *set;
    MulqUnit *unit;
    int ret;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zobj) == FAILURE) {
        RETURN_FALSE;
    }
    
    is_set = instanceof_function(Z_OBJCE_P(zobj), mulqset_ce TSRMLS_CC);
    is_unit = instanceof_function(Z_OBJCE_P(zobj), mulqunit_ce TSRMLS_CC);
    if (!is_set && !is_unit) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The arg must be an MulqSet or MulqUnit object");
        RETURN_FALSE;
    }
    
    mulq = (Mulq *)zend_object_store_get_object(object TSRMLS_CC);
    if (is_set) {
        set = (MulqSet *)zend_object_store_get_object(zobj TSRMLS_CC);
        ret = mulq_attach(mulq, set, sizeof(MulqSet **));
    } else {
        unit = (MulqUnit *)zend_object_store_get_object(zobj TSRMLS_CC);
        ret = mulq_attach(mulq, unit, sizeof(MulqUnit **));
    }
    
    if (ret) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}
/* }}} */

/* {{{ Mulq run
 */
PHP_METHOD(Mulq, run)
{
    zval *object = getThis();
    Mulq *mulq;
    
    mulq = (Mulq *)zend_object_store_get_object(object TSRMLS_CC);
    if (mulq_run(mulq) == SUCCESS) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ MuqlSet attach
 */
PHP_METHOD(MulqSet, attach)
{
    zval *zobj, *object = getThis();
    MulqSet *set;
    MulqUnit *unit;
    
    if (ZEND_NUM_ARGS() != 1 || 
        zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zobj) == FAILURE) {
        RETURN_FALSE;
    }
    
    if (!instanceof_function(Z_OBJCE_P(zobj), mulqunit_ce TSRMLS_CC)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The arg must be an MulqUnit object");
        RETURN_FALSE;
    }
    
    set = zend_object_store_get_object(object TSRMLS_CC);
    unit = zend_object_store_get_object(zobj TSRMLS_CC);
    if (mulq_set_attach(set, unit) == SUCCESS) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ MulqSet bind
 */
PHP_METHOD(MulqSet, bind)
{
    zval **data, *object = getThis();
    MulqSet *set;
    
    if (ZEND_NUM_ARGS() != 1 || 
        zend_get_parameters_ex(ZEND_NUM_ARGS(), &data) == FAILURE) {
		RETURN_FALSE;
	}
    
    if (!HASH_OF(*data)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The argument must be an array");
		RETURN_FALSE;
    }
    
    set = zend_object_store_get_object(object TSRMLS_CC);
    if (mulq_set_container(set, (void *)(*data)) == SUCCESS) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ MulqUnit server
 */
PHP_METHOD(MulqUnit, server)
{
    zval *zobj, *object = getThis();
    MulqUnit *unit;
    MulqServer *server;
    
    if (ZEND_NUM_ARGS() != 1 || 
        zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zobj) == FAILURE) {
        RETURN_FALSE;
    }
    
    if (!instanceof_function(Z_OBJCE_P(zobj), mulqserver_ce TSRMLS_CC)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The argument must be an MulqServer object");
		RETURN_FALSE;
    }
    
    unit = (MulqUnit *)zend_object_store_get_object(object TSRMLS_CC);
    server = (MulqServer *)zend_object_store_get_object(zobj TSRMLS_CC);
    if (mulq_unit_server(unit, server) == SUCCESS) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/* }} */

/* {{{ MulqUnit bind
 */
PHP_METHOD(MulqUnit, bind)
{
    zval *object = getThis();
    zval **sql, **data, **callback;
    HashTable *ht_sql;
    
    MulqUnit *unit;
    
    switch (ZEND_NUM_ARGS()) {
        case 3:
            if (zend_get_parameters_ex(ZEND_NUM_ARGS(), &sql, &callback, &data) == FAILURE)
                RETURN_FALSE;
            break;
        case 2:
            if (zend_get_parameters_ex(ZEND_NUM_ARGS(), &sql, &callback) == FAILURE)
                RETURN_FALSE;
            break;
        default:
            RETURN_FALSE;
    }
    
    ht_sql = HASH_OF(*sql);
    if (!ht_sql) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The argument1 must be an array");
        RETURN_FALSE;
    }
    
    if (!zend_is_callable(*callback, 0, NULL)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The argument2 must be callable");
        RETURN_FALSE;
    }
    
    unit = (MulqUnit *)zend_object_store_get_object(object TSRMLS_CC);
    
    if (ZEND_NUM_ARGS() == 3) {
        if (!HASH_OF(*data)) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "The argument3 must be an array");
            RETURN_FALSE;
        }
        
        mulq_unit_container(unit, (void *)(*data));
    } else {
        mulq_unit_container(unit, NULL);
    }
    
    mulq_unit_callback(unit, php_mulqunit_callback, callback, unit TSRMLS_CC);
    if (mulq_unit_sql(unit, ht_sql) == SUCCESS) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ MulqServer host
 */
PHP_METHOD(MulqServer, host)
{
    zval *host, *object = getThis();
    HashTable *ht_host;
    HashPosition pos;
    
    zval **v, **zhost, **zuser, **zpass, **zport;
    char *cuser, *cpass;
    uint cport;
    
    char *key;
    uint key_len;
    ulong h;
    
    MulqServer *server;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &host) == FAILURE) {
        RETURN_FALSE;
    }
    
    ht_host = HASH_OF(host);
    if (ht_host->nNumOfElements == 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The array can`t be empty");
        RETURN_FALSE;
    }
    
    server = (MulqServer *)zend_object_store_get_object(object TSRMLS_CC);
    mulq_server_clean(server);
    for (zend_hash_internal_pointer_reset_ex(ht_host, &pos);
         zend_hash_get_current_data_ex(ht_host, (void **)&v, &pos) == SUCCESS;
         zend_hash_move_forward_ex(ht_host, &pos)
    ) {
        if (Z_TYPE_PP(v) != IS_ARRAY || 
            zend_hash_find(Z_ARRVAL_PP(v), "host", 5, (void **)&zhost) != SUCCESS || 
            Z_TYPE_PP(zhost) != IS_STRING) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "The array must be two-dimensional array containning 'host' key");
            continue;
        }
        
        if (zend_hash_find(Z_ARRVAL_PP(v), "user", 5, (void **)&zuser) == SUCCESS) {
            cuser = Z_STRVAL_PP(zuser);
        } else {
            cuser = NULL;
        }
        
        if (zend_hash_find(Z_ARRVAL_PP(v), "pass", 5, (void **)&zpass) == SUCCESS) {
            cpass = Z_STRVAL_PP(zpass);
        } else {
            cpass = NULL;
        }
        
        if (zend_hash_find(Z_ARRVAL_PP(v), "port", 5, (void **)&zport) == SUCCESS) {
            cport = Z_LVAL_PP(zport);
        } else {
            cport = 0;
        }
        
        if (zend_hash_get_current_key_ex(ht_host, &key, &key_len, &h, 0, &pos) != HASH_KEY_IS_STRING) 
            key_len = 0;
        
        if (mulq_server_host(server, key, key_len, h, Z_STRVAL_PP(zhost), cuser, cpass, cport) != SUCCESS) 
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "The array:%s of host is invalid", Z_STRVAL_PP(zhost));
    }
    zend_hash_internal_pointer_reset_ex(ht_host, &pos);
    RETURN_TRUE;
}
/* }}} */
        
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
