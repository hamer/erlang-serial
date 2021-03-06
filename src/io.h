#ifndef __IO_H__
#define __IO_H__

#include <stdlib.h>

#ifdef WIN32
#   include <windows.h>
#endif

typedef enum { SPNONE, SPODD, SPEVEN } sparity_t;

typedef struct {
    char path[32];
    int baudrate;
    char databits, stopbits, flowcontrol;
    sparity_t parity;
} portconf_t;

typedef struct {
#ifdef WIN32
    HANDLE fd;
    OVERLAPPED ov_read, ov_write;
#else
    int fd;
#endif

    char req[2], repl[2]; // [0] -- read, [1] -- write
} context_t;

context_t *io_stdin(void);
context_t *io_stdout(void);
context_t *io_open_rs232(const portconf_t *pconf);

void io_close_rs232(context_t *ctx);
int io_poll(context_t *fds[], unsigned n);

ssize_t io_read(context_t *ctx, void *ptr, ssize_t len);
ssize_t io_write(context_t *ctx, const void *ptr, ssize_t len);

int io_set_baudrate(const context_t *ctx, int baudrate);
int io_set_databits(const context_t *ctx, int databits);
int io_set_parity(const context_t *ctx, sparity_t parity);
int io_set_stopbits(const context_t *ctx, int stopbits);
int io_set_flowcontrol(const context_t *ctx, int flowcontrol);

int io_enumerate(char *buf, size_t len);

#endif
