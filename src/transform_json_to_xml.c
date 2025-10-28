/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#include "external/mongoose.h"

#include "bridge.h"

/*--------------------------------------------------------------------------------------------------------------------*/

struct nyx_j2x_ctx_s
{
    nyx_j2x_emit_fn emit_fn;
};

/*--------------------------------------------------------------------------------------------------------------------*/

static bool key_eq(struct mg_str str, STR_t s)
{
    size_t n = strlen(s);

    return str.len == n + 2
           &&
           str.buf[0x000000000] == '"'
           &&
           str.buf[str.len - 1] == '"'
           &&
           memcmp(str.buf + 1, s, n) == 0
    ;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static bool key_is_attr(struct mg_str str)
{
    return str.len >= 4
           &&
           str.buf[0x000000000] == '"'
           &&
           str.buf[str.len - 1] == '"'
           &&
           str.buf[1] == '@'
    ;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static str_t dup_json_string(struct mg_str str)
{
    str_t result;

    /*----------------------------------------------------------------------------------------------------------------*/

    STR_t s = str.buf;
    size_t n = str.len;

    if(n >= 2
       &&
       s[0x000] == '"'
       &&
       s[n - 1] == '"'
    ) {
        /*------------------------------------------------------------------------------------------------------------*/

        struct mg_str inner = mg_str_n(
            s + 1,
            n - 2
        );

        result = malloc(inner.len + 1);

        if(!mg_json_unescape(inner, result, inner.len + 1))
        {
            memcpy(result, inner.buf, inner.len);

            result[inner.len] = '\0';
        }

        /*------------------------------------------------------------------------------------------------------------*/
    }
    else
    {
        /*------------------------------------------------------------------------------------------------------------*/

        result = strndup(s, n);

        /*------------------------------------------------------------------------------------------------------------*/
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    return result;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void j2x_emit_element(nyx_string_builder_t *out, struct mg_str obj)
{
    struct mg_str k, v;

    /*----------------------------------------------------------------------------------------------------------------*/

    struct mg_str tag_js = (struct mg_str) {0};
    struct mg_str txt_js = (struct mg_str) {0};
    struct mg_str kids_js = (struct mg_str) {0};

    for(size_t offset = 0; (offset = mg_json_next(obj, offset, &k, &v)) != 0;)
    {
        /*--*/ if(tag_js.len == 0 && key_eq(k, "<>")) {
            tag_js = v;
        } else if(txt_js.len == 0 && key_eq(k, ("$"))) {
            txt_js = v;
        } else if(kids_js.len == 0 && key_eq(k, "children")) {
            kids_js = v;
        }
    }

    if(tag_js.len == 0)
    {
        return;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    str_t tag = dup_json_string(tag_js);

    nyx_string_builder_append(out, false, false, "<", tag);

    for(size_t offset = 0; (offset = mg_json_next(obj, offset, &k, &v)) != 0; )
    {
        if(key_is_attr(k))
        {
            str_t aval = dup_json_string(v);

            nyx_string_builder_append(out, false, false, " ");
            nyx_string_builder_append_buff(out, k.len - 3, k.buf + 2, false, false);
            nyx_string_builder_append(out, false, false, "=\"");
            nyx_string_builder_append(out, false, true , aval);
            nyx_string_builder_append(out, false, false, "\"");

            free(aval);
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    bool has_txt = txt_js.len > 0;

    bool has_kids = kids_js.len > 0;

    /*----------------------------------------------------------------------------------------------------------------*/

    if(has_txt || has_kids)
    {
        nyx_string_builder_append(out, false, false, ">");

        if(has_txt)
        {
            str_t tv = dup_json_string(txt_js);
            nyx_string_builder_append(out, false, true, tv);
            free(tv);
        }

        if(has_kids)
        {
            for(size_t offset = 0; (offset = mg_json_next(kids_js, offset, &k, &v)) != 0;)
            {
                j2x_emit_element(out, v);
            }
        }

        nyx_string_builder_append(out, false, false, "</", tag, ">");
    }
    else
    {
        nyx_string_builder_append(out, false, false, "/>");
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    free(tag);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/
/* API                                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

nyx_j2x_ctx_t *nyx_j2x_init(nyx_j2x_emit_fn emit_fn)
{
    nyx_j2x_ctx_t *ctx = malloc(sizeof(nyx_j2x_ctx_t));

    memset(ctx, 0x00, sizeof(*ctx));

    ctx->emit_fn = emit_fn;

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
    if(ctx->emit_fn != NULL && len > 0x00 && text != NULL)
    {
        nyx_string_builder_t *sb = nyx_string_builder_new();

        j2x_emit_element(sb, mg_str_n(text, len));

        str_t xml = nyx_string_builder_to_string(sb);
        ctx->emit_fn(strlen(xml), xml);
        free(xml);

        nyx_string_builder_free(sb);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
