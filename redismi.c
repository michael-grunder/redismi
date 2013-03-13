#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Eclipse is balls
#ifndef NULL
#define NULL   ((void *) 0)
#endif

#include "redismi.h"
#include <zend_exceptions.h>

/*
 * Class entry pointer itself
 */
zend_class_entry *redismi_ce;

/*
 * Our exceptions
 */
zend_class_entry *redismi_exception_ce;
//zend_class_entry *spl_ce_RuntimeException = NULL;

/*
 * RedisMI function table
 */
static zend_function_entry redismi_methods[] = {
    PHP_ME(RedisMI, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, __destruct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, GetInfo, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, SetInfo, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, GetBuffer, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, SaveBuffer, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, SaveCallback, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMI, truncate, NULL, ZEND_ACC_PUBLIC)

    /* Pass-thru methods */
    #include "pt_struct.h"

    {NULL, NULL, NULL}
};

/* Pass-thru method implementation */
#include "pt_impl.h"

/*
 * Our own exceptions  -- CG(class_table) not found!!?!?!?!?!?!?!
 */
 /*
PHPAPI zend_class_entry *redismi_get_exception_base(int root TSRMLS_DC) {
#if HAVE_SPL
    if(!root) {
        if(!spl_ce_RuntimeException) {
            zend_class_entry **pce;

            TSRMSLS_FETCH();

            if(zend_hash_find(GC(class_table), 
                              "runtimeexception", sizeof("runtimeexception"),
                              (void**)&pce) == SUCCESS)
            {
                spl_ce_RuntimeException = *pce;
                return *pce;
            }
        } else {
            return spl_ce_RuntimeException;
        }
    }
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2) 
    return zend_exception_get_default();
#else
    return zend_exception_get_default(TSRMLS_C);
#endif
}
*/

/*
 * Create context structure
 */
zend_object_value create_redismi_context(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;
    redismi_context *context;

    // Allocate the actual struct
    context = (redismi_context*)emalloc(sizeof(redismi_context));
    memset(context, 0, sizeof(redismi_context));

    zend_object_std_init(&context->std, class_type TSRMLS_CC);

#if PHP_VERSION_ID < 50399
    zval *tmp;

    zend_hash_copy(context->std.properties,
        &class_type->default_properties,
        (copy_ctor_func_t) zval_add_ref,
        (void*)&tmp,
        sizeof(zval *)
    );
#else
    object_properties_init(&(context->std), class_type);
#endif

    retval.handle = zend_objects_store_put(
        context, 
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        free_redismi_context,
        NULL TSRMLS_CC
    );

    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

/* 
 * Destroy object context
 */
void free_redismi_context(void *object TSRMLS_DC) {
    // Grab context
    redismi_context *context = (redismi_context*)object;

    // Free our command buffer
    if(context->buf) {
        cb_free(context->buf);
    }

    // If we've got a context zval attached to our struct, free it
    if(context->z_info) {
    	zval_dtor(context->z_info);
    	efree(context->z_info);
    }

    // If we've got an attached callback, free our handler struct
    if(context->fci) {
    	efree(context->fci);
    }

    // Free context structure itself
    efree(context);
}

/*
 * Initialize our class and register it with PHP
 */
void init_redismi(TSRMLS_D) {
    zend_class_entry ce;
    
    /*
     * RedisMI
     */

    // Initialize our class entry
    INIT_CLASS_ENTRY(ce, "RedisMI", redismi_methods);

    // Set our function pointer to our context creation function
    ce.create_object = create_redismi_context;

    // Register the class with PHP
    redismi_ce = zend_register_internal_class(&ce TSRMLS_CC);

    // Properties
    //zend_declare_property_string(redismi_ce, "server", strlen("server"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
    //zend_declare_property_long(redismi_ce, "port", strlen("port"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);

    /* 
     * RedisMIException -- figure this nonsense out some other time
     */
/*
    // Initialize the exception class entry
    INIT_CLASS_ENTRY(ce_ex, "RedisMIException", NULL);
    redismi_exception_ce = zend_register_internal_class_ex(
        &ce_ex,
        redismi_get_exception_base(0 TSRMLS_CC),
        NULL TSRMLS_CC
    );
    */
}

/*
 * Construct a redis command, generically
 */
PHPAPI void redis_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, size_t cmd_len) {
    zval **z_args;
    int argc = ZEND_NUM_ARGS(), i;
    redismi_context *context = GET_CONTEXT();

    if(!context->buf) {
        zend_throw_exception(NULL, "Invalid command buffer", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        RETURN_FALSE;
    }

    // Command header
    cb_put_redis_hdr(context->buf, cmd, cmd_len, argc);

    // Iterate our arguments
    for(i = 0; i<argc; i++) {
        convert_to_string(z_args[i]);
        cb_put_redis_cmdl(context->buf, Z_STRVAL_P(z_args[i]), Z_STRLEN_P(z_args[i]));
    }

    // Increase the number of commands we've got in the buffer
    context->cmd_count++;

    // Free our argument array
    efree(z_args);

    // Success
    RETURN_TRUE;
}

/*
 * Set object's context value
 */
PHPAPI void set_object_info(INTERNAL_FUNCTION_PARAMETERS, redismi_context *context, zval *z_info) {
	// Free previous context if it's set
	if(context->z_info) {
		zval_dtor(context->z_info);
		efree(context->z_info);
		context->z_info = NULL;
	}

	// If the info is non null, copy it and set.  Otherwise null it out
	if(z_info) {
		MAKE_STD_ZVAL(context->z_info);
		*context->z_info = *z_info;
		zval_copy_ctor(context->z_info);
	}
}

/*
 * Call our attached callback
 */
PHPAPI int exec_save_callback(INTERNAL_FUNCTION_PARAMETERS, redismi_context *context,
							  char *file, int file_len)
{
   	zend_fcall_info        *fci = &context->fci->fci;
   	zend_fcall_info_cache  *fcc = &context->fci->fcc;
   	zval                  **params[3], *z_ret=NULL, *z_file, *z_cmds;

   	// RedisMI
   	params[0] = &getThis();

   	// Save file
   	MAKE_STD_ZVAL(z_file);
   	ZVAL_STRINGL(z_file, file, file_len, 1);
   	params[1] = &z_file;

   	// Number of commands
   	MAKE_STD_ZVAL(z_cmds);
   	ZVAL_LONG(z_cmds, context->cmd_count);
   	params[2] = &z_cmds;

   	// Set up call structure
   	fci->retval_ptr_ptr = &z_ret;
   	fci->params 		= params;
   	fci->param_count 	= 3;
   	fci->no_separation 	= 0;

   	// Call our function
   	int result = zend_call_function(fci, fcc TSRMLS_CC) != SUCCESS;

   	// Free our arguments
   	efree(z_file);
   	efree(z_cmds);

    // Free any return value the userland callback decided to create
   	if(z_ret) zval_ptr_dtor(&z_ret);

    // Success!
    return result;
}

/*
 * Method implementation
 */


PHP_METHOD(RedisMI, __construct) {
    // Initial buffer size
	zval *z_context = NULL;

    // See if we've been given a different initial size
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &z_context) == FAILURE) {
        RETURN_FALSE;
    }

    // Grab our context structure
    redismi_context *context = GET_CONTEXT();
    
    // If the user provided context, make a copy and attach it to our class
    if(z_context) {
        set_object_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, context, z_context);
    }

    // Create our command buffer
    context->buf = cb_init(INITIAL_BUFFER_SIZE);
    
    // Something is very wrong if we can't do that
    if(!context->buf) {
        zend_throw_exception(NULL, "Failed to create context buffer.  Possible OOM error", 0 TSRMLS_CC);
    }
    
    // Success
    RETURN_TRUE;
}

PHP_METHOD(RedisMI, __destruct) {
    // Success
    RETURN_TRUE;
}

/*
 * Get user assigned object info
 */
PHP_METHOD(RedisMI, GetInfo) {
	// Grab object context
	redismi_context *context = GET_CONTEXT();

	// If the context is non-null, return it
	if(context->z_info != NULL) {
		RETURN_ZVAL(context->z_info, 1, 0);
	} else {
		RETURN_NULL();
	}
}

/*
 * Set user assigned object info
 */
PHP_METHOD(RedisMI, SetInfo) {
	zval *z_info=NULL;

	// Make sure we can parse parameters
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_info) == FAILURE) {
		RETURN_FALSE;
	}

	redismi_context *context = GET_CONTEXT();

	// Udpate our object info
	set_object_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, context, z_info);

	RETURN_TRUE;
}

/*
 * Set a callback for when we've finished saving a buffer
 */
PHP_METHOD(RedisMI, SaveCallback) {
    // Allocate memory for our fcall info and cache
    callback_handler *cfi = emalloc(sizeof(*cfi));

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f", &cfi->fci, &cfi->fcc) == FAILURE) {
		efree(cfi);
        RETURN_FALSE;
	}

    if(cfi->fci.function_name) {
        Z_ADDREF_P(cfi->fci.function_name);
    }
    if(cfi->fci.object_ptr) {
        Z_ADDREF_P(cfi->fci.object_ptr);
    }

	// Grab our object context
	redismi_context *context = GET_CONTEXT();

	// Set our callback info in our context struct
	context->fci = cfi;
}

/*
 * Truncate our command buffer (doesn't free memory)
 */
PHP_METHOD(RedisMI, truncate) {
	int b_free = 0;

	// Make sure we can parse parameters
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &b_free) == FAILURE) {
		RETURN_FALSE;
	}

	// Get the classes context object
	redismi_context *context = GET_CONTEXT();

	// If we've been instructed to free memory associated with our command buffer, do that
	if(b_free) {
		// Free our buffer
		cb_free(context->buf);

		// Attempt to create a new buffer
		if(!(context->buf = cb_init(INITIAL_BUFFER_SIZE))) {
	        // Something is wrong
			zend_throw_exception(NULL, "Couldn't reallocate command buffer!", 0 TSRMLS_C);
			RETURN_FALSE;
		}
	}

	// Reset buffer position, and command count
    context->buf->pos  = 0;
    context->cmd_count = 0;

    // Success!
    RETURN_TRUE;
}

/*
 * Save the command buffer to a file
 */
PHP_METHOD(RedisMI, SaveBuffer) {
    char *file;
    int file_len, flush = 1;
    FILE *fp;

    // Make sure we were passed a filename
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &file, &file_len) == FAILURE) {
        RETURN_FALSE;
    }

    // Attempt to open the file
    if(!(fp = fopen(file, "w"))) {
        zend_throw_exception(NULL, "Couldn't open file", 0 TSRMLS_C);
        RETURN_FALSE;
    }

    // Grab our context
    redismi_context *context = GET_CONTEXT();

    // Write our file
    if(fwrite(context->buf->buf, 1, context->buf->pos, fp) < CMD_BUFLEN(context)) {
        zend_throw_exception(NULL, "Couldn't write buffer", 0 TSRMLS_CC);
        fclose(fp);
        RETURN_FALSE;
    }

    // Reset the buffer if we should
    if(flush) context->buf->pos = 0;

    // Close our file
    fclose(fp);

    // If we've got a callback, call it
    if(context->fci) {
    	if(exec_save_callback(INTERNAL_FUNCTION_PARAM_PASSTHRU, context, file, file_len) == FAILURE) {
            zend_throw_exception(NULL, "Couldn't execute callback!", 0 TSRMLS_CC);
            RETURN_FALSE;
    	}
    }

    // Success!
    RETURN_TRUE;
}

/*
 * Get the current command buffer as a string
 */
PHP_METHOD(RedisMI, GetBuffer) {
    redismi_context *context = GET_CONTEXT();
    RETURN_STRINGL(context->buf->buf, context->buf->pos, 1);
}