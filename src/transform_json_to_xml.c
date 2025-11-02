/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

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
           str.buf[1] == /*-*/ '@' /*-*/
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

        result = nyx_memory_alloc(inner.len + 1);

        if(mg_json_unescape(inner, result, inner.len + 1) == false)
        {
            memcpy(result, inner.buf, inner.len);

            result[inner.len] = '\0';
        }

        /*------------------------------------------------------------------------------------------------------------*/
    }
    else
    {
        /*------------------------------------------------------------------------------------------------------------*/

        result = nyx_string_ndup(s, n);

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

    struct mg_str tag_str = (struct mg_str) {0};
    struct mg_str txt_str = (struct mg_str) {0};
    struct mg_str kids_str = (struct mg_str) {0};

    for(size_t offset = 0; (offset = mg_json_next(obj, offset, &k, &v)) != 0;)
    {
        /*--*/ if(tag_str.len == 0 && key_eq(k, "<>")) {
            tag_str = v;
        } else if(txt_str.len == 0 && key_eq(k, /**/"$"/**/)) {
            txt_str = v;
        } else if(kids_str.len == 0 && key_eq(k, "children")) {
            kids_str = v;
        }
    }

    if(tag_str.len == 0)
    {
        return;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    str_t tag = dup_json_string(tag_str);

    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, "<", tag);

    for(size_t offset = 0; (offset = mg_json_next(obj, offset, &k, &v)) != 0; )
    {
        if(key_is_attr(k))
        {
            str_t val = dup_json_string(v);

            nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, " ");
            nyx_string_builder_append_buff(out, NYX_SB_NO_ESCAPE, k.len - 3, k.buf + 2);
            nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, "=\"");
            nyx_string_builder_append(out, NYX_SB_ESCAPE_XML, val);
            nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, "\"");

            nyx_memory_free(val);
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    bool has_txt = txt_str.len > 0;

    bool has_kids = kids_str.len > 0;

    /*----------------------------------------------------------------------------------------------------------------*/

    if(has_txt || has_kids)
    {
        nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, ">");

        if(has_txt)
        {
            str_t txt = dup_json_string(txt_str);
            nyx_string_builder_append(out, NYX_SB_ESCAPE_XML, txt);
            nyx_memory_free(txt);
        }

        if(has_kids)
        {
            for(size_t offset = 0; (offset = mg_json_next(kids_str, offset, &k, &v)) != 0;)
            {
                j2x_emit_element(out, v);
            }
        }

        nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, "</", tag, ">");
    }
    else
    {
        nyx_string_builder_append(out, NYX_SB_NO_ESCAPE, "/>");
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_memory_free(tag);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/
/* API                                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

nyx_j2x_ctx_t *nyx_j2x_init(nyx_j2x_emit_fn emit_fn)
{
    nyx_j2x_ctx_t *result = nyx_memory_alloc(sizeof(nyx_j2x_ctx_t));

    memset(result, 0x00, sizeof(nyx_j2x_ctx_t));

    result->emit_fn = emit_fn;

    return result;
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_j2x_close(nyx_j2x_ctx_t *ctx)
{
    nyx_memory_free(ctx);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_j2x_feed(const nyx_j2x_ctx_t *ctx, size_t len, STR_t text)
{
    if(ctx->emit_fn != NULL && len > 0x00 && text != NULL)
    {
        nyx_string_builder_t *sb = nyx_string_builder_new();

        j2x_emit_element(sb, mg_str_n(text, len));

        str_t xml = nyx_string_builder_to_string(sb);
        ctx->emit_fn(strlen(xml), xml);
        nyx_memory_free(xml);

        nyx_string_builder_free(sb);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
