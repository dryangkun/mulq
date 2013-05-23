#include "mulq.h"

static void *mulq_unit_thread_handler(void *arg) {
    MYSQL_RES *result;

    MYSQL_ROW row;
    ulong *rlength;

    MYSQL_FIELD *field;
    uint fnum;

    char **fname;
    uint *flength;

    int i = -1;

    MulqThread *thread;
    MulqUnit *unit;
    MulqHost *host;
    
    thread = (MulqThread *)arg;
    unit = thread->unit;
    host = thread->host;
    
    thread->tid = pthread_self();
    thread->flag |= MULQ_THREAD_STARTED;

#ifdef ZTS
    TSRMLS_D = unit->TSRMLS_C;
#endif
    
    mysql_thread_init();
    do {
        if (!(thread->flag & MULQ_THREAD_CONNECTED)) {
            if (NULL == mysql_real_connect(&thread->mysql, host->host, host->user, host->pass, NULL, host->port, host->unix_socket, 0))
                break;
            thread->flag |= MULQ_THREAD_CONNECTED;
        }

        if (mysql_query(&thread->mysql, thread->sql) != 0)
            break;

        result = mysql_store_result(&thread->mysql);
        if (result == NULL)
            break;

        fnum = mysql_num_fields(result);
        field = mysql_fetch_fields(result);

        fname = (char **)malloc(sizeof (char *) * fnum);
        if (fname == NULL) {
            mysql_free_result(result);
            break;
        }

        flength = (uint *)malloc(sizeof (uint) * fnum);
        if (flength == NULL) {
            mysql_free_result(result);
            free(fname);
            break;
        }

        for (i = 0; i < fnum; ++i) {
            fname[i] = field[i].name;
            flength[i] = field[i].name_length;
        }

        pthread_mutex_lock(unit->lock);
        while ((row = mysql_fetch_row(result))) {
            rlength = mysql_fetch_lengths(result);
            unit->callback(unit->data, (const char **) row, rlength, (const char **) fname, flength, fnum, unit->arg TSRMLS_CC);
        }
        pthread_mutex_unlock(unit->lock);

        mysql_free_result(result);
        free(fname);
        free(flength);
    } while (0);

    if (i == -1) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "The query fail:%s", mysql_error(&thread->mysql));
    }

    mysql_thread_end();
    pthread_exit((void *) 0);
}

static void *mulq_unit_run(void *unit) {
    MulqUnit *_unit;
    MulqThread **thread, **threads;

    HashTable *ht, *ht_sql;
    HashPosition pos;
    uint size_sql, i = 0;
    char *key;
    uint key_len;
    ulong h;

    zval **sql;

    memcpy(&_unit, &unit, sizeof (MulqUnit *));
    ht = &(_unit->ht);
    ht_sql = &(_unit->ht_sql);
    size_sql = ht_sql->nNumOfElements;
    
    if (_unit->mulq != NULL) {
        _unit->tid = pthread_self();
        _unit->flag |= MULQ_THREAD_STARTED;
    }
    
    if (ht->nNumOfElements == 0 || size_sql == 0) {
        if (_unit->mulq != NULL) {
            pthread_exit((void *) 0);
        } else {
            return ((void *) 0);
        }
    }

    threads = (MulqThread **)malloc(sizeof(MulqThread *) * ht->nNumOfElements);
    if (threads == NULL) {
        if (_unit->mulq != NULL) {
            pthread_exit((void *) 1);
        } else {
            return ((void *) 0);
        }
    }
    memset(threads, '\0', sizeof(MulqThread *) * ht->nNumOfElements);
    
    _unit->run_thread = 0;
    for (zend_hash_internal_pointer_reset_ex(ht, &pos);
        zend_hash_get_current_data_ex(ht, (void **) &thread, &pos) == SUCCESS;
        zend_hash_move_forward_ex(ht, &pos)
    ) {
        if (i >= size_sql)
            break;

        sql = NULL;
        if (zend_hash_get_current_key_ex(ht, &key, &key_len, &h, 0, &pos) == HASH_KEY_IS_STRING &&
            zend_hash_find(ht_sql, key, key_len, (void **) &sql) == FAILURE) {
            h = i;
        }

        if (sql == NULL && zend_hash_index_find(ht_sql, h, (void **) &sql) == FAILURE)
            continue;

        threads[i] = *thread;
        threads[i]->sql = Z_STRVAL_PP(sql);
        threads[i]->flag ^= MULQ_THREAD_STARTED;
        pthread_create(&(*thread)->tid, NULL, mulq_unit_thread_handler, (void *)threads[i]);
        ++i;
    }
    zend_hash_internal_pointer_reset_ex(ht, &pos);
    _unit->run_thread = i;

    if (_unit->mulq != NULL) {
        while (i--) {
            while (!(threads[i]->flag & MULQ_THREAD_STARTED));
            pthread_join(threads[i]->tid, NULL);
        }
        free(threads);
        pthread_exit((void *) 0);
    } else {
        return ((void *)threads);
    }
}

static void *mulq_set_run(void *unit) {
    MulqSet *_set;
    MulqUnit **_unit;
    MulqThread ***threads, *thread;

    HashTable *ht;
    HashPosition pos;

    pthread_mutex_t **lock;
    void **data, *rt;
    int i = 0, j;
    
    memcpy(&_set, &unit, sizeof (MulqSet *));
    ht = &(_set->ht);
    
    _set->tid = pthread_self();
    _set->flag |= MULQ_THREAD_STARTED;

    if (ht->nNumOfElements == 0)
        pthread_exit((void *) 0);

    threads = (MulqThread ***)malloc(sizeof (MulqThread **) * ht->nNumOfElements);
    if (threads == NULL)
        pthread_exit((void *) 1);
    memset(threads, '\0', sizeof (MulqThread **) * ht->nNumOfElements);
    
    data = (void **)malloc(sizeof(void *) * ht->nNumOfElements);
    if (data == NULL) {
        free(threads);
        pthread_exit((void *) 1);
    }
    
    lock = (pthread_mutex_t **)malloc(sizeof(pthread_mutex_t *) * ht->nNumOfElements);
    if (lock == NULL) {
        free(threads), free(data);
        pthread_exit((void *) 1);
    }

    for (zend_hash_internal_pointer_reset_ex(ht, &pos);
         zend_hash_get_current_data_ex(ht, (void **) &_unit, &pos) == SUCCESS;
         zend_hash_move_forward_ex(ht, &pos)
    ) {
        data[i] = (*_unit)->data;
        lock[i] = (*_unit)->lock;
        
        (*_unit)->lock = _set->lock;
        (*_unit)->data = _set->data;
        threads[i++] = (MulqThread **)(*_unit)->run((*_unit));
    }
    zend_hash_internal_pointer_reset_ex(ht, &pos);

    while (i--) {
        j = 0;
        thread = NULL;
        
        while (threads[i][j]) {
            while (!(threads[i][j]->flag & MULQ_THREAD_STARTED));
            
            thread = threads[i][j];
            pthread_join(threads[i][j++]->tid, &rt);
            
            if (j == thread->unit->run_thread)
                break;
        }
        
        if (thread) {
            thread->unit->data = data[i];
            thread->unit->lock = lock[i];
        }
        
        free(threads[i]);
    }
    free(threads);
    free(data);
    free(lock);
    pthread_exit((void *) 0);
}

static void mulq_unit_clean_thread(MulqUnit *unit) {
    MulqThread **thread;
    HashPosition pos;

    if (unit->ht.nNumOfElements > 0) {
        for (zend_hash_internal_pointer_reset_ex(&(unit->ht), &pos);
                zend_hash_get_current_data_ex(&(unit->ht), (void **) &thread, &pos) == SUCCESS;
                zend_hash_move_forward_ex(&(unit->ht), &pos)
                ) {
            if (thread != NULL) {
                if ((*thread)->flag & MULQ_THREAD_CONNECTED)
                    mysql_close(&(*thread)->mysql);
                free(*thread);
            }
        }

        zend_hash_clean(&(unit->ht));
    }
}

static void mulq_unit_clean_sql(MulqUnit *unit) {
    zend_hash_clean(&(unit->ht_sql));
}

static int mulq_host_init(MulqHost **host) {
    *host = (MulqHost *) malloc(sizeof (MulqHost));
    if (*host == NULL) {
        free(host);
        return -1;
    }

    memset((*host)->host, '\0', MULQ_HOST_SIZE);
    memset((*host)->user, '\0', MULQ_HOST_SIZE);
    memset((*host)->pass, '\0', MULQ_HOST_SIZE);
    (*host)->port = 0;
    memset((*host)->unix_socket, '\0', MULQ_HOST_SIZE);
    return 0;
}

static int mulq_host_update(MulqHost *host, const char *h, const char *u, const char *p, uint P) {
    if (h == NULL)
        return -1;

    if (h[0] != ':') {
        strncpy(host->host, h, MULQ_HOST_SIZE - 1);
        memset(host->unix_socket, '\0', MULQ_HOST_SIZE);
    } else {
        memset(host->host, '\0', MULQ_HOST_SIZE);
        strncpy(host->unix_socket, ++h, MULQ_HOST_SIZE - 1);
    }

    if (u != NULL)
        strncpy(host->user, u, MULQ_HOST_SIZE - 1);
    else
        strncpy(host->user, "root", MULQ_HOST_SIZE - 1);

    if (p != NULL)
        strncpy(host->pass, p, MULQ_HOST_SIZE - 1);
    else
        memset(host->pass, '\0', MULQ_HOST_SIZE);

    host->port = P > 0 ? P : 3306;
    return 0;
}

static int mulq_host_destroy(MulqHost **host) {
    free(*host);
    *host = NULL;
    return 0;
}

int mulq_server_init(MulqServer **server) {
    *server = (MulqServer *) malloc(sizeof (MulqServer));
    if (*server == NULL)
        return -1;

    return zend_hash_init(&((*server)->ht), 0, NULL, NULL, 0);
}

int mulq_server_host(MulqServer *server, char *key, uint key_len, ulong h, const char *host, const char *user, const char *pass, uint port) {
    MulqHost *_host;
    HashTable *ht;
    int ret;

    if (mulq_host_init(&_host) != 0)
        return -1;

    if (mulq_host_update(_host, host, user, pass, port) != 0) {
        mulq_host_destroy(&_host);
        return -1;
    }

    ht = &(server->ht);
    if (key_len == 0) {
        ret = zend_hash_index_update(ht, h, (void *) &_host, sizeof (MulqHost **), NULL);
    } else {
        ret = zend_hash_quick_update(ht, key, key_len, h, (void *) &_host, sizeof (MulqHost **), NULL);
    }
    return ret;
}

int mulq_server_clean(MulqServer *server) {
    HashTable *ht;
    HashPosition pos;

    MulqHost **host;

    ht = &server->ht;
    for (zend_hash_internal_pointer_reset_ex(ht, &pos);
            zend_hash_get_current_data_ex(ht, (void **) &host, &pos) == SUCCESS;
            zend_hash_move_forward_ex(ht, &pos)
            ) {
        if (host != NULL)
            mulq_host_destroy(host);
    }
    zend_hash_clean(ht);
    return 0;
}

int mulq_server_destroy(MulqServer **server) {
    mulq_server_clean(*server);
    zend_hash_destroy(&(*server)->ht);
    free(*server);
    *server = NULL;
    return 0;
}

int mulq_unit_init(MulqUnit **unit) {
    *unit = (MulqUnit *) malloc(sizeof (MulqUnit));
    if (*unit == NULL)
        return -1;

    (*unit)->mulq = NULL;
    (*unit)->h = -1;
    (*unit)->data = NULL;
    (*unit)->flag = 0;

    (*unit)->lock = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init((*unit)->lock, NULL);
    (*unit)->result = NULL;
    (*unit)->run = mulq_unit_run;

    zend_hash_init(&(*unit)->ht, 0, NULL, NULL, 0);

    zend_hash_init(&(*unit)->ht_sql, 0, NULL, ZVAL_PTR_DTOR, 0);
    (*unit)->callback = NULL;
    (*unit)->arg = NULL;
    (*unit)->run_thread = 0;

    return 0;
}

int mulq_unit_server(MulqUnit *unit, MulqServer *server) {
    MulqHost **host;
    MulqThread *thread;
    HashPosition pos;
    HashTable *ht_unit, *ht_server;

    ulong h;
    char *key;
    uint key_len;
    int ret = 0;

    ht_unit = &(unit->ht);
    ht_server = &server->ht;

    //when unit clear all server
    mulq_unit_clean_thread(unit);

    //server can`t be empty
    if (ht_server->nNumOfElements == 0)
        return -1;

    for (zend_hash_internal_pointer_reset_ex(ht_server, &pos);
            zend_hash_get_current_data_ex(ht_server, (void **) &host, &pos) == SUCCESS;
            zend_hash_move_forward_ex(ht_server, &pos)
            ) {
        thread = (MulqThread *) malloc(sizeof (MulqThread));
        if (thread == NULL || host == NULL) {
            ret = -1;
            break;
        }

        thread->unit = unit;
        thread->host = *host;
        thread->sql = NULL;
        thread->flag = 0;

        memset(&thread->mysql, '\0', sizeof (MYSQL));

        my_bool my_true = 0;
        uint ctimeout = 10;
        uint rtimeout = 60;

        mysql_options(&thread->mysql, MYSQL_SET_CHARSET_NAME, "utf8");
        mysql_options(&thread->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &ctimeout);
        mysql_options(&thread->mysql, MYSQL_OPT_RECONNECT, &my_true);
        mysql_options(&thread->mysql, MYSQL_OPT_READ_TIMEOUT, &rtimeout);

        if (zend_hash_get_current_key_ex(ht_server, &key, &key_len, &h, 0, &pos) == HASH_KEY_IS_STRING) {
            ret = zend_hash_update(ht_unit, key, key_len, (void *) &thread, sizeof (MulqThread **), NULL);
        } else {
            ret = zend_hash_index_update(ht_unit, h, (void *) &thread, sizeof (MulqThread **), NULL);
        }

        if (ret == FAILURE) {
            ret = -1;
            break;
        }
    }
    zend_hash_internal_pointer_reset_ex(ht_server, &pos);

    return ret;
}

int mulq_unit_sql(MulqUnit *unit, HashTable *ht_sql) {
    mulq_unit_clean_sql(unit);

    if (ht_sql->nNumOfElements == 0)
        return -1;

    zend_hash_copy(&unit->ht_sql, ht_sql, (copy_ctor_func_t) zval_add_ref, NULL, sizeof (zval));
    return 0;
}

int mulq_unit_callback(MulqUnit *unit, MULQ_UNIT_CALLBACK callback, zval **func_name, void *arg TSRMLS_DC) {
    if (callback == NULL)
        return -1;

    unit->callback = callback;
    unit->arg = arg;
#ifdef ZTS
    unit->TSRMLS_C = TSRMLS_C;
#endif
    
    if (unit->flag & MULQ_UNIT_CALLBACKED)
        zval_dtor(&unit->func_name);
    
    unit->func_name = **func_name;
    zval_copy_ctor(&unit->func_name);
    unit->flag |= MULQ_UNIT_CALLBACKED;
    return 0;
}

int mulq_unit_container(MulqUnit *unit, void *data) {
    if (data == NULL)
        unit->data = NULL;
    else
        unit->data = data;
    return 0;
}

int mulq_unit_destroy(MulqUnit **unit) {
    mulq_unit_clean_thread(*unit);
    zend_hash_destroy(&(*unit)->ht);
    zend_hash_destroy(&(*unit)->ht_sql);
    pthread_mutex_destroy((*unit)->lock);
    free((*unit)->lock);
    
    if ((*unit)->flag & MULQ_UNIT_CALLBACKED)
        zval_dtor(&(*unit)->func_name);
    
    free(*unit);
    *unit = NULL;
    return 0;
}

int mulq_set_init(MulqSet **set) {
    *set = (MulqSet *) malloc(sizeof (MulqSet));
    if (*set == NULL)
        return -1;

    (*set)->mulq = NULL;
    (*set)->h = -1;
    (*set)->data = NULL;
    (*set)->flag = 0;

    (*set)->lock = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init((*set)->lock, NULL);
    (*set)->result = NULL;
    (*set)->run = mulq_set_run;

    return zend_hash_init(&(*set)->ht, 0, NULL, NULL, 0);
}

int mulq_set_container(MulqSet *set, void *data) {
    if (data == NULL)
        return -1;

    set->data = data;
    return 0;
}

int mulq_set_attach(MulqSet *set, MulqUnit *unit) {
    HashTable *ht;
    int ret;

    if (unit->flag & MULQ_UNIT_ATTACHED)
        return -1;
    
    ht = &(set->ht);
    ret = zend_hash_next_index_insert(ht, (void **)&unit, sizeof (MulqUnit **), NULL);
    if (ret == SUCCESS) {
        unit->flag |= MULQ_UNIT_ATTACHED;
        unit->h = ht->nNumOfElements - 1;
        unit->mulq = NULL;
    }

    return ret;
}

int mulq_set_clean(MulqSet *set) {
    HashTable *ht;
    HashPosition pos;
    
    MulqUnit **unit;
    
    ht = &(set->ht);
    for (zend_hash_internal_pointer_reset_ex(ht, &pos);
         zend_hash_get_current_data_ex(ht, (void **)&unit, &pos) == SUCCESS;
         zend_hash_move_forward_ex(ht, &pos)
    ) {
        if (unit != NULL) {
            (*unit)->flag ^= MULQ_UNIT_ATTACHED;
            (*unit)->h = -1;
        }
    }
    zend_hash_clean(ht);
    return 0;
}

int mulq_set_destroy(MulqSet **set) {
    zend_hash_destroy(&(*set)->ht);
    pthread_mutex_destroy((*set)->lock);
    free((*set)->lock);
    free(*set);
    *set = NULL;
    return 0;
}

int mulq_init(Mulq **mulq) {
    *mulq = (Mulq *) malloc(sizeof (Mulq));
    if (*mulq == NULL)
        return -1;

    return zend_hash_init(&(*mulq)->ht, 0, NULL, NULL, 0);
}

int mulq_attach(Mulq *mulq, void *unit, uint size) {
    HashTable *ht;
    int ret;

    MulqSet *set;
    
    memcpy(&set, &unit, sizeof(MulqSet *));
    if (set->flag & MULQ_UNIT_ATTACHED)
        return -1;

    ht = &(mulq->ht);
    ret = zend_hash_next_index_insert(ht, (void *) &unit, size, NULL);
    if (ret == SUCCESS) {
        set->flag |= MULQ_UNIT_ATTACHED;
        set->mulq = mulq;
        set->h = ht->nNumOfElements - 1;
    }

    return ret;
}

int mulq_run(Mulq *mulq) {
    HashTable *ht;
    HashPosition pos;

    MulqSet *set, **sets;
    void **unit;
    
    uint i = 0, ret = 0;
    void *rt;

    ht = &mulq->ht;
    if (ht->nNumOfElements == 0)
        return -1;

    sets = (MulqSet **) malloc(sizeof (MulqSet *) * ht->nNumOfElements);
    if (sets == NULL)
        return -1;
    
    mysql_library_init(0, NULL, NULL);
    for (zend_hash_internal_pointer_reset_ex(ht, &pos);
            zend_hash_get_current_data_ex(ht, (void **) &unit, &pos) == SUCCESS;
            zend_hash_move_forward_ex(ht, &pos)
            ) {
        if (unit != NULL) {
            set = (MulqSet *) (*unit);
            sets[i++] = set;
            set->flag ^= MULQ_THREAD_STARTED;
            pthread_create(&set->tid, NULL, set->run, (void *)*unit);
        }
    }
    zend_hash_internal_pointer_reset_ex(ht, &pos);
    
    while (i--) {
        while (!(sets[i]->flag & MULQ_THREAD_STARTED));
        
        pthread_join(sets[i]->tid, &rt);
        if ((rt) != 0)
            ret = -1;
    }
    free(sets);
    mysql_library_end();
    
    return ret;
}

int mulq_clean(Mulq *mulq) {
    HashTable *ht;
    HashPosition pos;
    
    MulqUnit **set;
    
    ht = &(mulq->ht);
    for (zend_hash_internal_pointer_reset_ex(ht, &pos);
         zend_hash_get_current_data_ex(ht, (void **)&set, &pos) == SUCCESS;
         zend_hash_move_forward_ex(ht, &pos)
    ) {
        if (set != NULL) {
            (*set)->flag ^= MULQ_UNIT_ATTACHED;
            (*set)->h = -1;
            (*set)->mulq = NULL;
        }
    }
    zend_hash_clean(ht);
    return 0;
}

int mulq_destroy(Mulq **mulq) {
    zend_hash_destroy(&(*mulq)->ht);
    free(*mulq);
    *mulq = NULL;
    return 0;
}
