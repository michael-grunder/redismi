dnl lines starting with "dnl" are comments

PHP_ARG_ENABLE(redismi, whether to enable RedisMI extension, [  --enable-redismi   Enable RedisMI extension])

if test "$PHP_REDISMI" != "no"; then
    dnl this defines the extension
    PHP_NEW_EXTENSION(redismi, php_redismi.c redismi.c cmd_buf.c, $ext_shared)
fi

