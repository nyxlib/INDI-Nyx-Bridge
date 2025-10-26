/* INDI→Nyx JSON Streamer (XML→JSON)
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <string.h>
#include <ctype.h>
#include <libxml/SAX2.h>

#include "bridge.h"

/*--------------------------------------------------------------------------------------------------------------------*/

#define NYX_X2J_MAX_DEPTH 32

/*--------------------------------------------------------------------------------------------------------------------*/

struct nyx_x2j_ctx_s
{
    /*----------------------------------------------------------------------------------------------------------------*/

    void *ctx;

    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_string_builder_t *sb;

    nyx_string_builder_t *txt_sb[NYX_X2J_MAX_DEPTH];
    unsigned char txt_has[NYX_X2J_MAX_DEPTH];

    int has_children;
    int need_comma;
    int depth;

    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_x2j_emit_fn emit;

    /*----------------------------------------------------------------------------------------------------------------*/
};

/*--------------------------------------------------------------------------------------------------------------------*/
/* SAX HANDLERS                                                                                                       */
/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_start(void *ud, const xmlChar *name, const xmlChar **atts)
{
    nyx_x2j_ctx_t *p = ud;

    /*----------------------------------------------------------------------------------------------------------------*/

    int d = p->depth;

    if(d != 0)
    {
        /**/ if(d == 1)
        {
            nyx_string_builder_clear(p->sb);
            p->has_children = 0;
            p->need_comma = 0;
        }
        else if(d == 2)
        {
            if(!p->has_children)
            {
                nyx_string_builder_append(p->sb, false, false, ",\"children\":[");
                p->has_children = 1;
                p->need_comma = 0;
            }
            else if(p->need_comma)
            {
                nyx_string_builder_append(p->sb, false, false, ",");
                p->need_comma = 0;
            }
        }
        else if(p->need_comma)
        {
            nyx_string_builder_append(p->sb, false, false, ",");
            p->need_comma = 0;
        }

        nyx_string_builder_append(p->sb, false, false, "{\"<>\":\"");
        nyx_string_builder_append(p->sb, true , false, (STR_t) name);
        nyx_string_builder_append(p->sb, false, false, "\"");

        if(atts != NULL)
        {
            for(int i = 0; atts[i + 0] != NULL && atts[i + 1] != NULL; i += 2)
            {
                nyx_string_builder_append(p->sb, false, false, ",\"@");
                nyx_string_builder_append(p->sb, true , false, (STR_t) atts[i + 0]);
                nyx_string_builder_append(p->sb, false, false, "\":\"");
                nyx_string_builder_append(p->sb, true , false, (STR_t) atts[i + 1]);
                nyx_string_builder_append(p->sb, false, false, "\"");
            }
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    if(!(d < 0 || d >= NYX_X2J_MAX_DEPTH))
    {
        if(p->txt_sb[d] == NULL)
        {
            p->txt_sb[d] = nyx_string_builder_new();
        }
        else
        {
            nyx_string_builder_clear(p->txt_sb[d]);
        }
        p->txt_has[d] = 0;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    p->depth++;

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_end(void *ud, const xmlChar *name __attribute__ ((unused)))
{
    nyx_x2j_ctx_t *p = ud;

    /*----------------------------------------------------------------------------------------------------------------*/

    p->depth--;

    if(p->depth < 0) p->depth = 0;

    int d = p->depth;

    if(d == 0)
    {
        return;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    if(d >= 0 && d < NYX_X2J_MAX_DEPTH && p->txt_has[d])
    {
        nyx_string_builder_append(p->sb, false, false, ",\"$\":\"");
        {
            str_t t = nyx_string_builder_to_string(p->txt_sb[d]);
            nyx_string_builder_append(p->sb, false, false, t);
            free(t);
        }
        nyx_string_builder_append(p->sb, false, false, "\"");
        p->txt_has[d] = 0;
        nyx_string_builder_clear(p->txt_sb[d]);
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    if(d == 1)
    {
        if(p->has_children) nyx_string_builder_append(p->sb, false, false, "]");
        nyx_string_builder_append(p->sb, false, false, "}");
        if(p->emit != NULL)
        {
            str_t out = nyx_string_builder_to_string(p->sb);
            p->emit(strlen(out), out);
            free(out);
        }
        p->has_children = 0;
        p->need_comma   = 0;
    }
    else
    {
        nyx_string_builder_append(p->sb, false, false, "}");
        p->need_comma = 1;
    }

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_text(void *ud, const xmlChar *str, int len)
{
    nyx_x2j_ctx_t *p = ud;

    /*----------------------------------------------------------------------------------------------------------------*/

    if(len <= 0)
    {
        return;
    }

    int idx = p->depth - 1;

    if(idx < 0 || idx >= NYX_X2J_MAX_DEPTH)
    {
        return;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    str_t s = (str_t) str + 0x00000;
    str_t e = (str_t) str + len - 1;

    while(s <= e && isspace((unsigned char) *s)) { *s = '\0'; s++; }
    while(e >= s && isspace((unsigned char) *e)) { *e = '\0'; e--; }

    if(s <= e)
    {
        nyx_string_builder_append(p->txt_sb[idx], true, false, s);
        p->txt_has[idx] = 1;
    }

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

static xmlSAXHandler g_sax = {
    .initialized  = XML_SAX2_MAGIC,
    .startElement = sax_start,
    .endElement   = sax_end,
    .characters   = sax_text,
};

/*--------------------------------------------------------------------------------------------------------------------*/
/* API                                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

nyx_x2j_ctx_t *nyx_x2j_init(nyx_x2j_emit_fn emit)
{
    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_x2j_ctx_t *result = malloc(sizeof(nyx_x2j_ctx_t));

    memset(result, 0x00, sizeof(nyx_x2j_ctx_t));

    /*----------------------------------------------------------------------------------------------------------------*/

    result->ctx = xmlCreatePushParserCtxt(&g_sax, result, "<stream>", 8, NULL);

    if(result->ctx != NULL)
    {
        xmlCtxtUseOptions(result->ctx, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
        result->sb = nyx_string_builder_new();
        result->emit = emit;
        return result;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    free(result);

    return NULL;

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_x2j_close(nyx_x2j_ctx_t *ctx)
{
    /*----------------------------------------------------------------------------------------------------------------*/

    if(ctx->ctx != NULL)
    {
        xmlParseChunk(ctx->ctx, "</stream>", 9, 1);

        xmlFreeParserCtxt(ctx->ctx);
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    if(ctx->sb)
    {
        nyx_string_builder_free(ctx->sb);
    }

    for(int i = 0; i < NYX_X2J_MAX_DEPTH; i++)
    {
        if(ctx->txt_sb[i] != NULL)
        {
            nyx_string_builder_free(ctx->txt_sb[i]);
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    free(ctx);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_x2j_feed(const nyx_x2j_ctx_t *ctx, size_t len, STR_t text)
{
    if(ctx->ctx != NULL && len > 0 && text != NULL)
    {
        xmlParseChunk(ctx->ctx, text, (int) len, 0);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
