#include "rpc.h"
#include "rpc_user_info.h"
#include "jsonrpc.h"
#include "bufqu.h"
#include "job.h"
#include "fdevents.h"
#include "log.h"
#include <string.h>

/* TODO: 要支持多进程模式 */

static int rpc_method_echo(json_t *param, json_t **result)
{
    *result = json_incref(param);
    return 0;
}

static struct jsonrpc_method_entry_t rpc_methods[] = {
    {"echo", rpc_method_echo, "o"},
    {"get.all.urlpolicy", rpc_get_all_users_url_policy, NULL},
    {"get.user.urlpolicy", rpc_get_user_url_policy, "o"},
    {0,0,0}
};

struct buf_dump_context {
    buffer_qu_t *buf;
};

#define BUF_DUMP_UNIT 32

static int json_dump_buffer(const char *buf, size_t size, void *data)
{
    struct buf_dump_context *ctx = data;
    size_t need;

    if (!buf || !ctx)
        return -1;

    need = size % BUF_DUMP_UNIT;
    if (need)
        need = size + BUF_DUMP_UNIT - need;

    if (!ctx->buf) {
        ctx->buf = buffer_qu_new(need);
        if (!ctx->buf) {
            log_debug("alloc %d buffer for dump failed", size);
            return -1;
        }
    }

    if (size > ctx->buf->sum_len - ctx->buf->used) {
        if (buffer_qu_realloc(ctx->buf, ctx->buf->sum_len + need) != 0) {
            log_debug("realloc dump buffer to increase %d failed", size);
            goto errbuf;
        }
    }

    memcpy(ctx->buf->buf+ctx->buf->used, buf, size);
    ctx->buf->used += size;
    return 0;

errbuf:
    buffer_qu_del(ctx->buf);
    ctx->buf = NULL;
    return -1;
}

int rpc_handle(connection_t *c, uint8_t *msg, int len)
{
    int ret;
    struct buf_dump_context ctx = {
        .buf = NULL,
    };

    ret = jsonrpc_handler((const char *)msg, len, rpc_methods, json_dump_buffer, &ctx);
    if (ret != 0) {
        log_warn("handl json rpc failed");
        return -1;
    }

    if (ctx.buf) {
        buffer_put_tail( c->wbuf.bufqu, ctx.buf );
        job_addconn(FDEVENT_OUT, c);
        log_trace("response json rpc, buffer len %d", ctx.buf->used);
    } else
        log_debug("rpc handle not response");

    return 0;
}
