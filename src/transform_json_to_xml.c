/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "external/mongoose.h"
#include "bridge.h"

/*--------------------------------------------------------------------------------------------------------------------*/

struct nyx_j2x_ctx_s
{
    nyx_j2x_emit_fn emit_fn;
};

/*--------------------------------------------------------------------------------------------------------------------*/
/* HELPERS                                                                                                            */
/*--------------------------------------------------------------------------------------------------------------------*/

static inline bool key_eq(struct mg_str k, STR_t s)
{
    size_t n = strlen(s);

    return k.len == n + 2
           &&
           k.buf[0x0000000] == '"'
           &&
           k.buf[k.len - 1] == '"'
           &&
           memcmp(k.buf + 1, s, n) == 0
    ;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static inline bool key_is_dollar(struct mg_str k)
{
    return k.len == 3 && k.buf[0] == '"' && k.buf[1] == '$' && k.buf[2] == '"';
}

/*--------------------------------------------------------------------------------------------------------------------*/

static inline bool key_is_attr(struct mg_str k)
{
    return k.len >= 4 && k.buf[0] == '"' && k.buf[1] == '@';
}

/*--------------------------------------------------------------------------------------------------------------------*/

static str_t dup_json_string(struct mg_str js)
{
    STR_t  s = js.buf;
    size_t n = js.len;

    if(n >= 2 && s[0] == '"' && s[n - 1] == '"')
    {
        struct mg_str inner = { .buf = (str_t) s + 1, .len = n - 2 };

        str_t buf = (str_t) malloc(inner.len + 1);

        size_t w = mg_json_unescape(inner, buf, inner.len + 1);

        if(w == 0)
        {
            memcpy(buf, inner.buf, inner.len);
            buf[inner.len] = 0;
        }

        return buf;
    }

    return strndup(s, n);
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void j2x_emit_element(nyx_string_builder_t *out, struct mg_str obj)
{
    struct mg_str k, v;

    struct mg_str tag_js  = (struct mg_str) {0};
    struct mg_str text_js = (struct mg_str) {0};
    struct mg_str kids_js = (struct mg_str) {0};

    /*----------------------------------------------------------------------------------------------------------------*/

    for(size_t off = 0; (off = mg_json_next(obj, off, &k, &v)) != 0; )
    {
        if(tag_js.len  == 0 && key_eq(k, "<>"))             tag_js  = v;
        else if(text_js.len == 0 && key_is_dollar(k))       text_js = v;
        else if(kids_js.len == 0 && key_eq(k, "children"))  kids_js = v;
    }

    if(tag_js.len == 0) return;

    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_string_builder_append(out, false, false, "<");
    nyx_string_builder_append_buff(out, false, false, tag_js.buf + 1, tag_js.len - 2);

    for(size_t off = 0; (off = mg_json_next(obj, off, &k, &v)) != 0; )
    {
        if(key_is_attr(k))
        {
            nyx_string_builder_append(out, false, false, " ");
            nyx_string_builder_append_buff(out, false, false, k.buf + 2, k.len - 3);
            nyx_string_builder_append(out, false, false, "=\"");

            str_t aval = dup_json_string(v);
            nyx_string_builder_append(out, false, true , aval);
            nyx_string_builder_append(out, false, false, "\"");
            free(aval);
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    bool has_text = (text_js.len > 0);
    bool has_kids = (kids_js.len > 0);

    if(!has_text && !has_kids)
    {
        nyx_string_builder_append(out, false, false, "/>");
        return;
    }

    nyx_string_builder_append(out, false, false, ">");

    if(has_text)
    {
        str_t tv = dup_json_string(text_js);
        nyx_string_builder_append(out, false, true, tv);
        free(tv);
    }

    if(has_kids)
    {
        struct mg_str ek, ev;

        for(size_t aoff = 0; (aoff = mg_json_next(kids_js, aoff, &ek, &ev)) != 0; )
        {
            j2x_emit_element(out, ev);
        }
    }

    nyx_string_builder_append(out, false, false, "</");
    nyx_string_builder_append_buff(out, false, false, tag_js.buf + 1, tag_js.len - 2);
    nyx_string_builder_append(out, false, false, ">");
}

/*--------------------------------------------------------------------------------------------------------------------*/
/* API                                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

nyx_j2x_ctx_t *nyx_j2x_init(nyx_j2x_emit_fn emit)
{
    nyx_j2x_ctx_t *ctx = (nyx_j2x_ctx_t *) malloc(sizeof(nyx_j2x_ctx_t));

    memset(ctx, 0x00, sizeof(*ctx));

    ctx->emit_fn = emit;

    return ctx;
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_j2x_close(nyx_j2x_ctx_t *ctx)
{
    free(ctx);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_j2x_feed(nyx_j2x_ctx_t *ctx, size_t len, STR_t text)
{
    if(ctx == NULL || text == NULL || len == 0) return;

    struct mg_str in = { .buf = (str_t) text, .len = len };

    nyx_string_builder_t *sb = nyx_string_builder_new();

    j2x_emit_element(sb, in);

    str_t xml = nyx_string_builder_to_string(sb);

    if(ctx->emit_fn != NULL)
    {
        ctx->emit_fn(strlen(xml), xml);
    }

    free(xml);

    nyx_string_builder_free(sb);
}

/*--------------------------------------------------------------------------------------------------------------------*/
