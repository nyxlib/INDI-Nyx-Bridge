/* INDI→Nyx JSON Streamer (XML→JSON)
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <ctype.h>
#include <string.h>
#include <stdbool.h>

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

    nyx_x2j_emit_fn emit;

    /*----------------------------------------------------------------------------------------------------------------*/

    int depth;
    nyx_string_builder_t *sb;

    int text_depth;
    nyx_string_builder_t *txt_sb;

    /*----------------------------------------------------------------------------------------------------------------*/

    bool has_children;
    bool has_text;
    bool comma;

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

    /*----------------------------------------------------------------------------------------------------------------*/

    /**/ if(d == 1)
    {
        nyx_string_builder_clear(p->sb);
        p->has_children = false;
        p->comma = false;
    }
    else if(d == 2)
    {
        if(!p->has_children)
        {
            nyx_string_builder_append(p->sb, false, false, ",\"children\":[");
            p->has_children = true;
        }

        if(p->comma)
        {
            nyx_string_builder_append(p->sb, false, false, ",");
            p->comma = false;
        }
    }
    else
    {
        goto __skip;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

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

    /*----------------------------------------------------------------------------------------------------------------*/

__skip:
    nyx_string_builder_clear(p->txt_sb);
    p->has_text = false;
    p->text_depth = d;

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

    if(p->depth < 0)
    {
        p->depth = 0;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    int d = p->depth;

    /*----------------------------------------------------------------------------------------------------------------*/

    if(p->has_text && p->text_depth == d)
    {
        str_t s = nyx_string_builder_to_string(p->txt_sb);
        nyx_string_builder_clear(p->txt_sb);

        nyx_string_builder_append(p->sb, false, false, ",\"$\":\"");
        nyx_string_builder_append(p->sb, true, false, s);
        nyx_string_builder_append(p->sb, false, false, "\"");

        p->has_text = false;

        free(s);
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    /**/ if(d == 1)
    {
        if(p->has_children)
        {
            nyx_string_builder_append(p->sb, false, false, "]");
        }

        nyx_string_builder_append(p->sb, false, false, "}");

        if(p->emit != NULL)
        {
            str_t out = nyx_string_builder_to_string(p->sb);
            p->emit(strlen(out), out);
            free(out);
        }

        p->has_children = false;
        p->comma = false;
    }
    else if(d == 2)
    {
        nyx_string_builder_append(p->sb, false, false, "}");
        p->comma = true;
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

    if(idx != p->text_depth)
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
        nyx_string_builder_append(p->txt_sb, true, false, s);
        p->has_text = true;
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

        result->emit = emit;

        result->sb = nyx_string_builder_new();

        result->txt_sb = nyx_string_builder_new();

        return result;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    free(result);

    /*----------------------------------------------------------------------------------------------------------------*/

    return NULL;
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

    if(ctx->sb != NULL) {
        nyx_string_builder_free(ctx->sb);
    }

    if(ctx->txt_sb != NULL) {
        nyx_string_builder_free(ctx->txt_sb);
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    free(ctx);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_x2j_feed(const nyx_x2j_ctx_t *ctx, size_t len, STR_t text)
{
    if(ctx->ctx != NULL && len > 0x00 && text != NULL)
    {
        xmlParseChunk(ctx->ctx, text, (int) len, 0);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
