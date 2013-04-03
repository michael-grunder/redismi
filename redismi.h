#include <php.h>
#include "cmd_buf.h"
#include <stdlib.h>

#ifndef REDISMI_H
#define REDISMI_H

/*
 * This macro allows us to implement every redis command by passing it into our
 * variable argument command processor redis_cmd.  We do this because using
 * the magic method __call is nearly 3x as slow.
 */
#define PASSTHRU_METHOD(name) \
    PHP_METHOD(RedisMI, name) { \
        redis_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, #name, sizeof(#name)-1); \
    }

/*
 * Context access helpers
 */
#define CMD_BUF(c) (c->buf)
#define CMD_BUFLEN(c) (c->buf->pos)

/*
 * Get our context, with much less typing
 */
#define GET_CONTEXT() (redismi_context*)zend_object_store_get_object(getThis() TSRMLS_CC)

/*
 * Initial buffer size
 */
#define INITIAL_BUFFER_SIZE 32768

/*
 * 
 */
typedef struct _callback_handler {
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
} callback_handler;

/* 
 * Context for our class
 */
typedef struct _redismi_context {
    // Standard required object
    zend_object std;

    // Any kind of information the user wants to attach to an instance
    zval *z_info;

    // Command buffer
    cmdbuf *buf;

    // Number of commands in our buffer
    int cmd_count;

    // The last error/command count for any sent buffers
    int send_err_count;
    int send_cmd_count;

    // Save callback handlers
    callback_handler *fci;

} redismi_context;

/* 
 * Create our own exceptions
 */
PHPAPI zend_class_entry *redismi_get_exception_base(int root TSRMLS_DC);

/*
 * Initialize / Free our structure
 */
zend_object_value create_redismi_context(zend_class_entry *class_type TSRMLS_DC);
void free_redismi_context(void *object TSRMLS_DC);

/*
 * Register the class with PHP
 */
void init_redismi(TSRMLS_D);

/*
 * Generic command construction
 */
PHPAPI void redis_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, size_t cmd_len);

/*
 * Set an object's info zval
 */
PHPAPI void set_object_info(INTERNAL_FUNCTION_PARAMETERS, redismi_context *ctx, zval *z_info);

/*
 * Execute our save callback
 */
PHPAPI int exec_save_callback(INTERNAL_FUNCTION_PARAMETERS, redismi_context *context, char *file, int file_len);

/*
 * Methods
 */
PHP_METHOD(RedisMI, __construct);
PHP_METHOD(RedisMI, __destruct);
PHP_METHOD(RedisMI, GetBuffer);
PHP_METHOD(RedisMI, SetBuffer);
PHP_METHOD(RedisMI, LoadBuffer);
PHP_METHOD(RedisMI, SaveBuffer);
PHP_METHOD(RedisMI, SendBuffer);
PHP_METHOD(RedisMI, GetInfo);
PHP_METHOD(RedisMI, SetInfo);
PHP_METHOD(RedisMI, SaveCallback);
PHP_METHOD(RedisMI, LastErrorCount);
PHP_METHOD(RedisMI, LastReplyCount);
PHP_METHOD(RedisMI, truncate);

/*
 * Pass thru methods.  These methods aren't really implemented except for
 * stubs which pass every command through redis_cmd.
 */
#include "pt_header.h"

#endif
