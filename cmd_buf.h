#ifndef CMD_BUG_H
#define CMD_BUF_H

#include <string.h> // For memcpy
#include <php.h>

/*
 * Command buffer structure
 */
typedef struct _cmd_buf {
    /*
     * The actual buffer
     */
    char *buf;
    
    /*
     * Buffer size
     */
    size_t size;

    /*
     * Our position
     */
    size_t pos;
} cmdbuf;

// When we're small, we want to aggressively grow our buffer
#define CB_MAX_PREALLOC (1024*1024)

// Grab the size of our buffer
#define CB_SIZE(p) (p->size)

// Grab the current length of our buffer
#define CB_LEN(p) (p->pos)

// Grab a pointer to the current position
#define CB_PTR(p) (p->buf+p->pos)

// How much space is left
#define CB_REM(p) (p->size-p->pos)

// Put a character at our position
#define CB_PUTC(p) (*CB_PTR(p)=c, CB_POS(p)++)

// 
// Init/Free methods
//

// Create a command buffer
cmdbuf *cb_init(size_t size);

// Free a command buffer
int cb_free(cmdbuf *buf);

// Make sure the buffer is at least size large
int cbuf_alloc(cmdbuf *buf, size_t size);

// Increase the size of our buffer
int cbuf_grow(cmdbuf *buf);

//
// Append to our buffer
//

// Append/put strings or characters
int cb_appendl(cmdbuf *buf, const char *str, unsigned int len);
int cb_append(cmdbuf *buf, const char *str);
int cb_putc(cmdbuf *buf, char c);

// Put numbers and such
int cb_put_int(cmdbuf *buf, int num);
int cb_put_uint(cmdbuf *buf, unsigned int num);
int cb_put_lng(cmdbuf *buf, long num);

//
// Redis specific stuff
//

int cb_put_redis_hdr(cmdbuf *buf, const char *cmd, unsigned int cmd_len, unsigned int num_args);
int cb_put_redis_cmdl(cmdbuf *buf, const char *cmd, unsigned int cmd_len);
int cb_put_redis_cmd(cmdbuf *buf, const char *cmd);

#endif 
