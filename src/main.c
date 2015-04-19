#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "io.h"

typedef enum {
    CENUMERATE      = 0,
    CBAUDRATE       = 1,
    CDATABITS       = 2,
    CPARITY         = 3,
    CSTOPBITS       = 4,
    CFLOWCONTROL    = 5,
    COPEN           = 6,
    CCLOSE          = 7,
    CSEND           = 8,
    CRECV           = 9,
    CERROR          = 10
} erl_cmd_l;

typedef struct {
    uint8_t len, done;
    enum { CHDR, CDATA, CREADY } state;
    char data[256];
} erl_chunk_t;

typedef struct {
    char data[1024];
    char *head, *tail, empty;
} cbuf_t;

ssize_t erl_read(context_t *ctx, erl_chunk_t *chunk);

size_t cbuf_used(cbuf_t *cbuf);
size_t cbuf_free(cbuf_t *cbuf);
void cbuf_norm(cbuf_t *cbuf);

int main(void) {
    erl_chunk_t erl_inp = { 0xFF, 0, CHDR, {0} };
    cbuf_t erl_outp, rs_outp;
    char rs_inp[256];
    context_t *ctx[3];
    portconf_t portconf = {
        .path = "",
        .baudrate = 115200,
        .databits = 8,
        .stopbits = 1,
        .flowcontrol = 0,
        .parity = SPNONE
    };
    ssize_t rs;

    erl_outp.head = erl_outp.tail = erl_outp.data;
    rs_outp.head  = rs_outp.tail  = rs_outp.data;
    erl_outp.empty = rs_outp.empty = 1;

    ctx[0] = io_stdin();
    ctx[1] = io_stdout();
    ctx[2] = NULL;

    for (;;) {
        if ((rs = erl_read(ctx[0], &erl_inp)) < 0 && rs == -1)
            return 1;
        else if (rs > 0 && erl_inp.state == CREADY) {
            erl_inp.state = CHDR;
            erl_inp.done = 0;
            switch (erl_inp.data[0]) {
            case CENUMERATE:
                {
                    char buf[256];

                    if (io_enumerate(buf, sizeof(buf)) < 0)
                        return 1;

                    size_t len = 1 + 1 + strlen(buf);

                    if (cbuf_free(&erl_outp) < len)
                        return 1;

                    erl_outp.head[0] = len - 1;
                    erl_outp.head[1] = CENUMERATE;
                    memcpy(erl_outp.head + 2, buf, len);
                    erl_outp.head += len;
                }
                break;

            case CBAUDRATE:
                erl_inp.data[erl_inp.len] = '\0';
                portconf.baudrate = atoi(erl_inp.data + 1);
                if (ctx[2]) io_set_baudrate(ctx[2], portconf.baudrate);
                break;

            case CFLOWCONTROL:
                portconf.flowcontrol = erl_inp.data[1];
                if (ctx[2]) io_set_flowcontrol(ctx[2], portconf.flowcontrol);
                break;

            case CDATABITS:
                portconf.databits = erl_inp.data[1];
                if (ctx[2]) io_set_databits(ctx[2], portconf.databits);
                break;

            case CPARITY:
                if (!strncmp(erl_inp.data + 1, "none", 4))
                    portconf.parity = SPNONE;
                else if (!strncmp(erl_inp.data + 1, "odd", 3))
                    portconf.parity = SPODD;
                else if (!strncmp(erl_inp.data + 1, "even", 3))
                    portconf.parity = SPEVEN;
                if (ctx[2]) io_set_parity(ctx[2], portconf.parity);
                break;

            case CSTOPBITS:
                portconf.stopbits = erl_inp.data[1];
                if (ctx[2]) io_set_stopbits(ctx[2], portconf.stopbits);
                break;

            case COPEN:
                strncpy(portconf.path, erl_inp.data + 1, sizeof(portconf.path) - 1);
                portconf.path[sizeof(portconf.path) - 1] = '\0';

                if (ctx[2])
                    io_close_rs232(ctx[2]);

                if (!(ctx[2] = io_open_rs232(&portconf)))
                    return 1;

                break;

            case CCLOSE:
                if (ctx[2]) {
                    io_close_rs232(ctx[2]);
                    ctx[2] = NULL;
                }
                break;

            case CSEND:
                if (cbuf_free(&rs_outp) < erl_inp.len)
                    return 1;

                memcpy(rs_outp.head, erl_inp.data + 1, erl_inp.len - 1);
                rs_outp.head += erl_inp.len - 1;
                break;
            }
        }

        if (ctx[2]) {
            if ((rs = io_read(ctx[2], rs_inp, sizeof(rs_inp))) < 0 && rs == -1)
                return 1;
            else if (rs > 0) {
                size_t len = 1 + 1 + rs;

                if (cbuf_free(&erl_outp) < len)
                    return 1;

                erl_outp.head[0] = len - 1;
                erl_outp.head[1] = CRECV;
                memcpy(erl_outp.head + 2, rs_inp, len);
                erl_outp.head += len;
            }

            while (ctx[2] && cbuf_used(&rs_outp)) {
                if ((rs = io_write(ctx[2], rs_outp.tail, cbuf_used(&rs_outp))) < 0 && rs == -1)
                    return 1;
                else if (rs > 0) {
                    rs_outp.tail += rs;
                    cbuf_norm(&rs_outp);
                } else
                    break;
            }
        }

        while (cbuf_used(&erl_outp)) {
            if ((rs = io_write(ctx[1], erl_outp.tail, cbuf_used(&erl_outp))) < 0 && rs == -1)
                return 1;
            else if (rs > 0) {
                erl_outp.tail += rs;
                cbuf_norm(&erl_outp);
            } else
                break;
        }

        if (!ctx[0]->req[0] || (ctx[2] && !ctx[2]->req[0]))
            continue;

        io_poll(ctx, 3);
    }

    return 0;
}

ssize_t erl_read(context_t *ctx, erl_chunk_t *chunk) {
    ssize_t rs;

    switch (chunk->state) {
    case CHDR:
        if ((rs = io_read(ctx, &chunk->len, 1)) <= 0)
            return rs == 0 ? -1 : rs;

        chunk->state = CDATA;
        chunk->done = 0;
        // no break
    case CDATA:
        if ((rs = io_read(ctx, chunk->data + chunk->done, chunk->len - chunk->done)) <= 0)
            return rs == 0 ? -1 : rs;

        chunk->done += rs;
        if (chunk->done == chunk->len)
            chunk->state = CREADY;
        break;

    case CREADY:
        break;
    }

    return chunk->state == CREADY;
}

size_t cbuf_used(cbuf_t *cbuf) {
    return cbuf->head - cbuf->tail;
}

size_t cbuf_free(cbuf_t *cbuf) {
    return sizeof(cbuf->data) / 2 - cbuf_used(cbuf);
}

void cbuf_norm(cbuf_t *cbuf) {
    static const size_t hsz = sizeof(cbuf->data) / 2;
    const char *hdata = cbuf->data + hsz;

    if (cbuf->tail >= hdata) {
        memcpy(cbuf->tail - hsz, cbuf->tail, cbuf->head - cbuf->tail);
        cbuf->head -= hsz;
        cbuf->tail -= hsz;
    }
}
