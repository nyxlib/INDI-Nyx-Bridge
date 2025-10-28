/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <ctype.h>
#include <string.h>

#include <libxml/SAX2.h>

#include "bridge.h"

/*--------------------------------------------------------------------------------------------------------------------*/

struct nyx_x2j_ctx_s
{
    /*----------------------------------------------------------------------------------------------------------------*/

    xmlParserCtxt *sax_ctx;

    /*----------------------------------------------------------------------------------------------------------------*/

    int depth;

    nyx_string_builder_t *sb;

    nyx_string_builder_t *txt_sb;

    /*----------------------------------------------------------------------------------------------------------------*/

    bool has_text;

    int children_cnt;

    nyx_x2j_emit_fn emit_fn;

    /*----------------------------------------------------------------------------------------------------------------*/
};

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_start(void *ud, const xmlChar *name, const xmlChar **atts)
{
    nyx_x2j_ctx_t *x2j_ctx = ud;

    /*----------------------------------------------------------------------------------------------------------------*/

    int d = x2j_ctx->depth++;

    /*----------------------------------------------------------------------------------------------------------------*/

    /**/ if(d == 1)
    {
        nyx_string_builder_clear(x2j_ctx->sb);

        x2j_ctx->children_cnt = 0;
    }
    else if(d == 2)
    {
        if(x2j_ctx->children_cnt == 0)
        {
            nyx_string_builder_append(x2j_ctx->sb, false, false, ",\"children\":[");
        }
        else
        {
            nyx_string_builder_append(x2j_ctx->sb, false, false, ",");
        }
    }
    else
    {
        goto __skip;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_string_builder_append(x2j_ctx->sb, false, false, "{\"<>\":\"");
    nyx_string_builder_append(x2j_ctx->sb, true , false, (STR_t) name);
    nyx_string_builder_append(x2j_ctx->sb, false, false, "\"");

    if(atts != NULL)
    {
        for(int i = 0; atts[i + 0] != NULL && atts[i + 1] != NULL; i += 2)
        {
            nyx_string_builder_append(x2j_ctx->sb, false, false, ",\"@");
            nyx_string_builder_append(x2j_ctx->sb, true , false, (STR_t) atts[i + 0]);
            nyx_string_builder_append(x2j_ctx->sb, false, false, "\":\"");
            nyx_string_builder_append(x2j_ctx->sb, true , false, (STR_t) atts[i + 1]);
            nyx_string_builder_append(x2j_ctx->sb, false, false, "\"");
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

__skip:
    nyx_string_builder_clear(x2j_ctx->txt_sb);

    x2j_ctx->has_text = false;

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_end(void *ud, __attribute__ ((unused)) const xmlChar *name)
{
    nyx_x2j_ctx_t *x2j_ctx = ud;

    /*----------------------------------------------------------------------------------------------------------------*/

    x2j_ctx->depth--;

    if(x2j_ctx->depth < 0)
    {
        x2j_ctx->depth = 0;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    int d = x2j_ctx->depth;

    /*----------------------------------------------------------------------------------------------------------------*/

    if(x2j_ctx->has_text)
    {
        str_t s = nyx_string_builder_to_string(x2j_ctx->txt_sb);

        nyx_string_builder_append(x2j_ctx->sb, false, false, ",\"$\":\"");
        nyx_string_builder_append(x2j_ctx->sb, true, false, s);
        nyx_string_builder_append(x2j_ctx->sb, false, false, "\"");

        nyx_string_builder_clear(x2j_ctx->txt_sb);

        x2j_ctx->has_text = false;

        free(s);
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    /**/ if(d == 1)
    {
        if(x2j_ctx->children_cnt > 0)
        {
            nyx_string_builder_append(x2j_ctx->sb, false, false, "]");
        }

        nyx_string_builder_append(x2j_ctx->sb, false, false, "}");

        x2j_ctx->children_cnt = 0;

        /**/

        if(x2j_ctx->emit_fn != NULL)
        {
            str_t out = nyx_string_builder_to_string(x2j_ctx->sb);
            x2j_ctx->emit_fn(strlen(out), out);
            free(out);
        }
    }
    else if(d == 2)
    {
        nyx_string_builder_append(x2j_ctx->sb, false, false, "}");

        x2j_ctx->children_cnt++;
    }

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_txt(void *ud, const xmlChar *str, int len)
{
    nyx_x2j_ctx_t *p = ud;

    /*----------------------------------------------------------------------------------------------------------------*/

    if(len <= 0 || p->depth != 3)
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

static xmlSAXHandler sax = {
    .initialized  = XML_SAX2_MAGIC,
    .startElement = sax_start,
    .endElement   = sax_end,
    .characters   = sax_txt,
};

/*--------------------------------------------------------------------------------------------------------------------*/
/* API                                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

nyx_x2j_ctx_t *nyx_x2j_init(nyx_x2j_emit_fn emit_fn)
{
    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_x2j_ctx_t *result = malloc(sizeof(nyx_x2j_ctx_t));

    memset(result, 0x00, sizeof(nyx_x2j_ctx_t));

    /*----------------------------------------------------------------------------------------------------------------*/

    result->sax_ctx = xmlCreatePushParserCtxt(&sax, result, "<stream>", 8, NULL);

    if(result->sax_ctx != NULL)
    {
        xmlCtxtUseOptions(result->sax_ctx, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    }
    else
    {
        free(result);

        return NULL;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    result->emit_fn = emit_fn;

    result->sb = nyx_string_builder_new();

    result->txt_sb = nyx_string_builder_new();

    /*----------------------------------------------------------------------------------------------------------------*/

    return result;
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_x2j_close(nyx_x2j_ctx_t *ctx)
{
    /*----------------------------------------------------------------------------------------------------------------*/

    if(ctx->sax_ctx != NULL)
    {
        xmlParseChunk(ctx->sax_ctx, "</stream>", 9, 1);

        xmlFreeParserCtxt(ctx->sax_ctx);
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
    if(ctx->emit_fn != NULL && len > 0x00 && text != NULL)
    {
        xmlParseChunk(ctx->sax_ctx, text, (int) len, 0);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
