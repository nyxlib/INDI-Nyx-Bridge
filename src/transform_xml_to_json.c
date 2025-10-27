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

struct nyx_x2j_ctx_s
{
    /*----------------------------------------------------------------------------------------------------------------*/

    xmlParserCtxt *ctx;

    /*----------------------------------------------------------------------------------------------------------------*/

    int depth;

    nyx_string_builder_t *sb;

    nyx_string_builder_t *txt_sb;

    /*----------------------------------------------------------------------------------------------------------------*/

    bool has_text;

    int children_cnt;

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

    int d = p->depth++;

    /*----------------------------------------------------------------------------------------------------------------*/

    /**/ if(d == 1)
    {
        nyx_string_builder_clear(p->sb);

        p->children_cnt = 0;
    }
    else if(d == 2)
    {
        if(p->children_cnt == 0)
        {
            nyx_string_builder_append(p->sb, false, false, ",\"children\":[");
        }
        else
        {
            nyx_string_builder_append(p->sb, false, false, ",");
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

    if(p->has_text)
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
        if(p->children_cnt > 0)
        {
            nyx_string_builder_append(p->sb, false, false, "]");
        }

        nyx_string_builder_append(p->sb, false, false, "}");

        p->children_cnt = 0;

        /**/

        if(p->emit != NULL)
        {
            str_t out = nyx_string_builder_to_string(p->sb);
            p->emit(strlen(out), out);
            free(out);
        }
    }
    else if(d == 2)
    {
        nyx_string_builder_append(p->sb, false, false, "}");

        p->children_cnt++;
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

nyx_x2j_ctx_t *nyx_x2j_init(nyx_x2j_emit_fn emit)
{
    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_x2j_ctx_t *result = malloc(sizeof(nyx_x2j_ctx_t));

    memset(result, 0x00, sizeof(nyx_x2j_ctx_t));

    /*----------------------------------------------------------------------------------------------------------------*/

    result->ctx = xmlCreatePushParserCtxt(&sax, result, "<stream>", 8, NULL);

    if(result->ctx != NULL)
    {
        xmlCtxtUseOptions(result->ctx, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    }
    else
    {
        free(result);

        return NULL;
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    result->emit = emit;

    result->sb = nyx_string_builder_new();

    result->txt_sb = nyx_string_builder_new();

    /*----------------------------------------------------------------------------------------------------------------*/

    return result;
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
