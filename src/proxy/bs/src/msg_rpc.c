#include "msg_rpc.h"
#include "rpc.h"
#include "fdrw.h"
#include "log.h"

static ssize_t json_reslove_one(uint8_t *buf, size_t len)
{
    struct json_reslover {
        unsigned begin:1;
        unsigned quote:1;
        unsigned slash:1;
        unsigned depth:29;
    } r = {0, 0, 0, 0};
    uint8_t *p;

    for (p=buf; p<buf+len; p++) {
        switch (*p) {
        case '{':
            if (!r.begin)
                r.begin = 1;
            if (!r.quote) {
                if (++r.depth == 0)
                    goto badmsg;
            }
            if (r.slash)
                r.slash = 0;
            break;

        case '}':
            if (!r.begin)
                goto badmsg;
            if (!r.quote) {
                if (--r.depth <= 0) {
                    r.begin = 0;
                    goto done;
                }
            }
            break;

        case '\\':
            if (!r.begin || !r.quote)
                goto badmsg;
            r.slash = 1;
            break;

        case '"':
            if (!r.begin)
                goto badmsg;
            if (r.slash)
                r.slash = 0;
            else
                r.quote = !r.quote;
            break;

        case ' ': case '\t': case '\r': case '\n':
        case '\0':
            break;

        default:
            if (!r.begin)
                goto badmsg;
            if (r.slash)
                r.slash = 0;
            break;
        }
    }
    return 0;

done:
    return p+1-buf;
badmsg:
    return buf-p-1;
}

int rpc_msg_reslove(connection_t *c)
{
    readbuf_t *rb = &c->rbuf;
    ssize_t len;

    if (rb->used == 0)
        goto errnomore;
    if (rb->pos >= rb->size)
        goto errmem;

    len = json_reslove_one(rb->buf+rb->pos, rb->used);
    if (len < 0) {
        log_warn("reslove json msg return %d(%c)",
                len, *(rb->buf + rb->pos - len -1));
        goto errpck;
    }
    if (len == 0)
    {
        if( rb->used >= rb->msgmax ) goto errpck;
        else goto errnomore;
    }

    log_trace("reslove one json msg, len %d", len);
    rb->msgbuf = rb->buf + rb->pos;
    rb->msglen = len;

    rb->pos += len;
    rb->used -= len;
    if (rb->used > 0)
        goto errmore;

    return FDRW_OK;

errmore:
    return FDRW_MOR;
errpck:
    return FDRW_PCK;
errmem:
    return FDRW_MEM;
errnomore:
    return FDRW_NOM;
}

int rpc_msg_parse( connection_t *c )
{
    return rpc_handle(c, c->rbuf.msgbuf, c->rbuf.msglen);
}
