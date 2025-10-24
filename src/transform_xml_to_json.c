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
    void *ctx;

    nyx_string_builder_t *sb;

    nyx_string_builder_t *txt_sb[NYX_X2J_MAX_DEPTH];
    unsigned char txt_has[NYX_X2J_MAX_DEPTH];

    int has_children;
    int need_comma;
    int depth;

    nyx_x2j_emit_fn emit;
};

/*--------------------------------------------------------------------------------------------------------------------*/
/* HELPERS                                                                                                            */
/*--------------------------------------------------------------------------------------------------------------------*/

static inline void sb_json(nyx_string_builder_t *dst, STR_t s, bool xml)
{
    nyx_string_builder_t *tmp = nyx_string_builder_new();

    if(xml) {
        nyx_string_builder_append_xml(tmp, s);
    }
    else {
        nyx_string_builder_append(tmp, s);
    }

    str_t js = nyx_string_builder_to_string(tmp);
    nyx_string_builder_append(dst, js);
    free(js);

    nyx_string_builder_free(tmp);
}

/*--------------------------------------------------------------------------------------------------------------------*/

static inline void txt_clear(nyx_x2j_ctx_t *p, int d)
{
    if(d < 0x0000000000000000
       ||
       d >= NYX_X2J_MAX_DEPTH
    ) {
        return;
    }

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

/*--------------------------------------------------------------------------------------------------------------------*/
/* SAX HANDLERS                                                                                                       */
/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_start(void *ud, const xmlChar *name, const xmlChar **atts)
{
    nyx_x2j_ctx_t *p = ud;

    int d = p->depth;

    if(d == 0)
    {
        /* racine factice <stream> */
    }
    else if(d == 1)
    {
        p->has_children = 0;
        p->need_comma = 0;

        nyx_string_builder_clear(p->sb);

        nyx_string_builder_append(p->sb, "{", "\"<>\":");
        sb_json(p->sb, (const char *) name, false);

        if(atts != NULL)
        {
            for(int i = 0; atts[i] && atts[i + 1]; i += 2)
            {
                nyx_string_builder_append(p->sb, ",", "\"@", (const char *) atts[i], "\"", ":");
                sb_json(p->sb, (const char *) atts[i + 1], true);
            }
        }
    }
    else
    {
        if(d == 2)
        {
            if(!p->has_children)
            {
                nyx_string_builder_append(p->sb, ",\"children\":[");
                p->has_children = 1;
                p->need_comma = 0;
            }
            else if(p->need_comma)
            {
                nyx_string_builder_append(p->sb, ",");
            }
        }
        else
        {
            nyx_string_builder_append(p->sb, ",");
        }

        nyx_string_builder_append(p->sb, "{", "\"<>\":");
        sb_json(p->sb, (const char *) name, false);

        if(atts != NULL)
        {
            for(int i = 0; atts[i] && atts[i + 1]; i += 2)
            {
                nyx_string_builder_append(p->sb, ",", "\"@", (const char *) atts[i], "\"", ":");
                sb_json(p->sb, (const char *) atts[i + 1], true);
            }
        }
    }

    txt_clear(p, d);

    p->depth++;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_text(void *ud, const xmlChar *ch, int len)
{
    nyx_x2j_ctx_t *p = ud;

    if(len <= 0)
    {
        return;
    }

    int ti = p->depth - 1;

    if(ti < 0 || ti >= NYX_X2J_MAX_DEPTH)
    {
        return;
    }

    char *s = (char *) ch + 0x00000;
    char *e = (char *) ch + len - 1;

    while(s <= e && isspace((unsigned char) *s)) { *s = '\0'; s++; }
    while(e >= s && isspace((unsigned char) *e)) { *e = '\0'; e--; }

    if(s > e)
    {
        return;
    }

    nyx_string_builder_append_xml(p->txt_sb[ti], s);

    p->txt_has[ti] = 1;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void sax_end(void *ud, const xmlChar *name __attribute__ ((unused)))
{
    nyx_x2j_ctx_t *p = ud;

    p->depth--;

    if(p->depth < 0) p->depth = 0;

    int d = p->depth;

    if(d == 0)
    {
        return;
    }

    if(d >= 0 && d < NYX_X2J_MAX_DEPTH && p->txt_has[d])
    {
        nyx_string_builder_append(p->sb, ",\"$\":");

        str_t js = nyx_string_builder_to_string(p->txt_sb[d]);

        nyx_string_builder_append(p->sb, js);

        free(js);

        p->txt_has[d] = 0;
        nyx_string_builder_clear(p->txt_sb[d]);
    }

    if(d == 1)
    {
        if(p->has_children) nyx_string_builder_append(p->sb, "]");

        nyx_string_builder_append(p->sb, "}");

        if(p->emit != NULL)
        {
            str_t out = nyx_string_builder_to_cstring(p->sb);
            p->emit(strlen(out), out);
            free(out);
        }

        p->has_children = 0;
        p->need_comma = 0;
    }
    else
    {
        nyx_string_builder_append(p->sb, "}");

        if(d == 2) p->need_comma = 1;
    }
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
