/*
 * cmd_buf.c 
 * 
 * Simple resizeable append buffer
 */

#include "cmd_buf.h"

/* 
 * Initialize our bufer
 */
cmdbuf* cb_init(size_t size) {
    // Allocate our buffer itself
    cmdbuf *retval = emalloc(sizeof(cmdbuf));
    
    // OOM
    if(!retval) {
        return NULL;
    }
    
    // Now allocate our buffer
    retval->buf = emalloc(size);

    // Must be OOM here
    if(!retval->buf) {
        return NULL;
    }

    // Set our size and position
    retval->size = size;
    retval->pos = 0;

    // Return our buffer
    return retval;
}

/*
 * Free a command buffer
 */
int cb_free(cmdbuf *buf) {
    // Make sure we have a buffer
    if(!buf) {
        return -1;
    }

    // Free our buffer if it's not null
    if(buf->buf) efree(buf->buf); 

    // Free buf structure 
    efree(buf);

    // Success
    return 0;
}

/*
 * Grow the size of our buffer.  We'll grow aggressively up to max
 * prealloc, then just add our prealloc max each time (until we OOM)
 */
int cb_grow(cmdbuf *buf) {
    // Make sure we were passed a buffer
    if(!buf) {
        return -1;
    }
    
    // Double our buffer size unless it's bigger than CB_MAX_PREALLOC
    // in which case we juse try to get that many more bytes.
    if(buf->size > CB_MAX_PREALLOC) {
        buf->size += CB_MAX_PREALLOC;
    } else {
        buf->size *= 2;
    }

    // Reallocate our buffer
    buf->buf = erealloc(buf->buf, buf->size);

    // Return failure if we ran out of memory
    if(!buf->buf) {
        return -1;
    }

    // Success
    return 0;
}

/*
 * Append a string to our buffer
 */
int cb_appendl(cmdbuf *buf, const char *str, unsigned int len) {
    // If we weren't passed a buffer, or we can't fit this new string in our 
    // buffer because it would put us past our maximum size, return failure.
    if(!buf || !str || (len >= CB_REM(buf) && cb_grow(buf) != 0)) {
        return -1;
    }

    // Append this string to our buffer
	memcpy(CB_PTR(buf), str, len);

    // Update our buffer position
    buf->pos += len;

    // Success
    return 0;
}

/* 
 * Append a string, without passing in the length
 */
int cb_append(cmdbuf *buf, const char *str) {
    return cb_appendl(buf, str, strlen(str));
}

/* 
 * Append a single character into our buffer
 */
int cb_putc(cmdbuf *buf, char c) {
    // NULL check
    if(!buf) {
        return -1;
    }
    
    // If we're out of space and we can't grow, fail
    if(!CB_REM(buf) && cb_grow(buf)) {
        return -1;
    }

    // Put our character
    buf->buf[buf->pos] = c;

    // Move forward
    buf->pos++;
}

/* 
 * Append a long
 */
int cb_put_int(cmdbuf *buf, int num) {
    char lng_buf[255];
    int len = snprintf(lng_buf, sizeof(lng_buf), "%ld", num);

    // Append our int 
    return cb_appendl(buf, lng_buf, len);
}

/*
 * Put an unsigned int
 */
int cb_put_uint(cmdbuf *buf, unsigned int num) {
    char uint_buf[255];
    int len = snprintf(uint_buf, sizeof(uint_buf), "%u", num);

    // Append our unsigned int
    return cb_appendl(buf, uint_buf, len);
}

/*
 * Put a long
 */
int cb_put_lng(cmdbuf *buf, long num) {
    char lng_buf[255];
    int len = snprintf(lng_buf, sizeof(lng_buf), "%ld", num);
    return cb_appendl(buf, lng_buf, len);
}

/*
 * Put a command header (e.g. *3\r\n$3\r\nGET\r\n$3\r\nFOO)
 */
int cb_put_redis_hdr(cmdbuf *buf, const char *cmd, unsigned int cmd_len, unsigned int num_args) {
    // Multibulk header
    cb_putc(buf, '*');

    // Number of commands to follow
    cb_put_uint(buf, num_args + 1);
    
    // CRLF
    cb_appendl(buf, "\r\n", 2);

    // Put the command itself
    cb_put_redis_cmdl(buf, cmd, cmd_len);

    return 0;
}

/*
 * Put a command part (e.g. $<cmd len>\r\n<cmd>\r\n)
 */
int cb_put_redis_cmdl(cmdbuf *buf, const char *cmd, unsigned int cmd_len) {
    // Tell reids how many bytes the next sequence will be
    cb_putc(buf, '$');
    cb_put_uint(buf, cmd_len);

    // CRLF
    cb_appendl(buf, "\r\n", 2);    

    // Append our command
    cb_appendl(buf, cmd, cmd_len);

    // CRLF
    cb_appendl(buf, "\r\n", 2);
}
