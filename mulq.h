/* 
 * File:   mulq.h
 * Author: dryoung
 *
 * Created on 2012年11月11日, 下午3:58
 */

#ifndef MULQ_H
#define	MULQ_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <pthread.h>
#include <sys/types.h>
#include <mysql.h>
#include "php.h"
#include <zend.h>
#include <zend_API.h>
#include <zend_hash.h>
#include <zend_alloc.h>

#ifdef ZTS
#include <TSRM.h>
#endif

#define MULQ_HOST_SIZE 30
#define MULQ_UNIT_ATTACHED 0x1
#define MULQ_UNIT_CALLBACKED 0x4
#define MULQ_THREAD_STARTED 0x2
#define MULQ_THREAD_CONNECTED 0x1
    
typedef void (*MULQ_UNIT_CALLBACK)(void *data, const char **row, ulong *rlength, const char **field, uint *flength, uint fnum, void *arg TSRMLS_DC);

typedef void *(*MULQ_RUN)(void *unit);
    
typedef struct _MulqHost {
    char host[MULQ_HOST_SIZE];
    char user[MULQ_HOST_SIZE];
    char pass[MULQ_HOST_SIZE];
    uint port;
    char unix_socket[MULQ_HOST_SIZE];
} MulqHost;
    
typedef struct _MulqServer {
    zend_object zobject;
    HashTable ht;
} MulqServer;

typedef struct _MulqThread {
    struct _MulqHost *host;
    char *sql;
    
    pthread_t tid;
    MYSQL mysql;
    uint flag;
    
    struct _MulqUnit *unit;
} MulqThread;

typedef struct _MulqUnit {
    zend_object zobject;
    HashTable ht;
    
    struct _Mulq *mulq;
    ulong h;
    void *data;
    uint flag;
    
    pthread_mutex_t *lock;
    MYSQL_RES *result;
    MULQ_RUN run;
    pthread_t tid;
    
    HashTable ht_sql;
    MULQ_UNIT_CALLBACK callback;
    void *arg;
    uint run_thread;
    
    zval func_name;
#ifdef ZTS
    TSRMLS_D;
#endif
} MulqUnit;

typedef struct _MulqSet {
    zend_object zobject;
    HashTable ht;
    
    struct _Mulq *mulq;
    ulong h;
    void *data;
    uint flag;
    
    pthread_mutex_t *lock;
    MYSQL_RES *result;
    MULQ_RUN run;
    pthread_t tid;
} MulqSet;

typedef struct _Mulq {
    zend_object zobject;
    HashTable ht;
} Mulq;

int mulq_server_init(MulqServer **server);
int mulq_server_host(MulqServer *server, char *key, uint key_len, ulong h, const char *host, const char *user, const char *pass, uint port);
int mulq_server_clean(MulqServer *server);
int mulq_server_destroy(MulqServer **server);

int mulq_unit_init(MulqUnit **unit);
int mulq_unit_server(MulqUnit *unit, MulqServer *server);
int mulq_unit_sql(MulqUnit *unit, HashTable *ht_sql);
int mulq_unit_callback(MulqUnit *unit, MULQ_UNIT_CALLBACK callback, zval **func_name, void *arg TSRMLS_DC);
int mulq_unit_container(MulqUnit *unit, void *data);
int mulq_unit_destroy(MulqUnit **unit);

int mulq_set_init(MulqSet **set);
int mulq_set_container(MulqSet *set, void *data);
int mulq_set_attach(MulqSet *set, MulqUnit *unit);
int mulq_set_clean(MulqSet *set);
int mulq_set_destroy(MulqSet **set);

int mulq_init(Mulq **mulq);
int mulq_attach(Mulq *mulq, void *unit, uint size);
int mulq_run(Mulq *mulq);
int mulq_clean(Mulq *mulq);
int mulq_destroy(Mulq **mulq);

#endif	/* MULQ_H */

