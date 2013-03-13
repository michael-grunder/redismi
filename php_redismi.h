#ifndef PHP_REDISMI_H
#define PHP_REDISMI_H

extern zend_module_entry redismi_module_entry;
#define redismi_module_ptr &redismi_module_entry
#define phpext_redismi_ptr redismi_module_ptr

#define PHP_REDISMI_VERSION "0.0.1"

/* 
 * Module init/shutdown stuff
 */
PHP_MINIT_FUNCTION(redismi);
//PHP_MSHUTDOWN_FUNCTION(redismi);
//PHP_RSHUTDOWN_FUNCTION(redismi);
PHP_MINFO_FUNCTION(redismi);

#endif
