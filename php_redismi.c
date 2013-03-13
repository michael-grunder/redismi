#include <php.h>
#include "php_redismi.h"
#include "redismi.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

zend_module_entry redismi_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "redismi",
    NULL,
    PHP_MINIT(redismi),
    NULL,                            //PHP_MSHUTDOWN(redismi),
    NULL,                            //PHP_RINIT(redismi),
    NULL,                            // PHP_RSHUTDOWN(redismi),
    PHP_MINFO(redismi),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_REDISMI_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_REDISMI
ZEND_GET_MODULE(redismi)
#endif

/**
 * Module initialization
 */
PHP_MINIT_FUNCTION(redismi) {
    // Initialize our redismi class
    init_redismi(TSRMLS_C);
    return 0;
}

/*
 * Module info 
 */
PHP_MINFO_FUNCTION(redismi) {
    php_info_print_table_start();
    php_info_print_table_header(2, "Redis mass import support", "enabled");
    php_info_print_table_row(2, "Redis mass import version", PHP_REDISMI_VERSION);
    php_info_print_table_end();
}
