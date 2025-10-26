/* JSON→XML (Nyx → INDI)
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
    nyx_j2x_emit_fn emit;
};

/*--------------------------------------------------------------------------------------------------------------------*/
/* HELPERS                                                                                                            */
/*--------------------------------------------------------------------------------------------------------------------*/

static inline int key_eq(struct mg_str k, STR_t s)
{
    /* k est une clé JSON QUOTÉE: "xxx" */
    size_t n = strlen(s);

    return k.len == n + 2
           &&
           k.buf[0] == '"'
           &&
           k.buf[k.len - 1] == '"'
           &&
           memcmp(k.buf + 1, s, n) == 0
    ;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static inline int key_starts_with_at(struct mg_str k)
{
    /* "@attr" => k = "\"@attr\"" */
    return k.len >= 3 && k.buf[0] == '"' && k.buf[1] == '@';
}

/*--------------------------------------------------------------------------------------------------------------------*/

static inline void key_attr_name(struct mg_str k, const char **name, size_t *len)
{
    /* transforme "\"@name\"" -> name="name", len=4 */
    *name = (const char *) (k.buf + 2);
    *len  = (size_t) (k.len - 3);
}

/*--------------------------------------------------------------------------------------------------------------------*/

static inline struct mg_str jget(struct mg_str json, const char *path)
{
    int toklen = 0;
    int off = mg_json_get(json, path, &toklen);

    return off < 0 ? (struct mg_str){.buf = NULL, .len = 0}
                   : (struct mg_str){.buf = json.buf + off, .len = (size_t) toklen};
}

/*--------------------------------------------------------------------------------------------------------------------*/

static str_t json_string_to_cstr(struct mg_str js)
{
    /* js peut être: "string" (avec guillemets) OU un token brut (nombre, true/false, etc.) */
    const char *s = js.buf;
    size_t      n = js.len;

    if(n >= 2 && s[0] == '"' && s[n - 1] == '"')
    {
        /* enlever les guillemets, puis unescape */
        struct mg_str inner = { .buf = (char *) s + 1, .len = n - 2 };

        char *buf = (char *) malloc(inner.len + 1);

        if(buf == NULL) return NULL;

        size_t w = mg_json_unescape(inner, buf, inner.len + 1);

        if(w == 0)
        {
            /* fallback: copie brute si unescape échoue */
            memcpy(buf, inner.buf, inner.len);
            buf[inner.len] = 0;
        }

        return buf;
    }
    else
    {
        /* nombre / bool / null → copie brute */
        char *buf = (char *) malloc(n + 1);

        if(buf == NULL) return NULL;

        memcpy(buf, s, n);
        buf[n] = 0;

        return buf;
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void j2x_emit_element(nyx_string_builder_t *out, struct mg_str obj);

/*--------------------------------------------------------------------------------------------------------------------*/

static void j2x_emit_children(nyx_string_builder_t *out, struct mg_str arr)
{
    size_t off = 0;

    struct mg_str k, v;

    while((off = mg_json_next(arr, off, &k, &v)) != 0)
    {
        j2x_emit_element(out, v);
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void j2x_emit_element(nyx_string_builder_t *out, struct mg_str obj)
{
    /*----------------------------------------------------------------------------------------------------------------*/
    /* "<>" (nom de balise)                                                                                           */
    /*----------------------------------------------------------------------------------------------------------------*/

    struct mg_str tag_js = jget(obj, "$.<>");

    if(tag_js.len == 0) return;

    str_t tag = json_string_to_cstr(tag_js);

    if(tag == NULL) return;

    /*----------------------------------------------------------------------------------------------------------------*/
    /* OUVERTURE + ATTRIBUTS                                                                                          */
    /*----------------------------------------------------------------------------------------------------------------*/

    nyx_string_builder_append(out, false, false, "<", tag);

    size_t off = 0;

    struct mg_str k, v;

    while((off = mg_json_next(obj, off, &k, &v)) != 0)
    {
        if(key_eq(k, "<>")
           ||
           key_eq(k, "$")
           ||
           key_eq(k, "children")
        ) {
            continue;
        }

        if(key_starts_with_at(k))
        {
            const char *an;
            size_t      al;

            key_attr_name(k, &an, &al);

            str_t av = json_string_to_cstr(v);

            if(av != NULL)
            {
                /* an n'est pas NUL-terminé → on duplique pour append() */
                char *name = (char *) malloc(al + 1);

                if(name != NULL)
                {
                    memcpy(name, an, al);
                    name[al] = 0;

                    nyx_string_builder_append(out, false, false, " ", name, "=\"");
                    nyx_string_builder_append(out, false, true , av);   /* xml=true pour échapper &,<,>,",\' */
                    nyx_string_builder_append(out, false, false, "\"");

                    free(name);
                }

                free(av);
            }
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    /* TEXTE + ENFANTS                                                                                                */
    /*----------------------------------------------------------------------------------------------------------------*/

    /* clé "$" → $.\"$\" dans mg_json_get */
    struct mg_str text_js = jget(obj, "$.\"$\"");
    struct mg_str kids_js = jget(obj, "$.children");

    int has_text = (text_js.len > 0);
    int has_kids = (kids_js.len > 0);

    if(!has_text && !has_kids)
    {
        nyx_string_builder_append(out, false, false, "/>");
        free(tag);
        return;
    }

    nyx_string_builder_append(out, false, false, ">");

    if(has_text)
    {
        str_t tv = json_string_to_cstr(text_js);

        if(tv != NULL)
        {
            nyx_string_builder_append(out, false, true, tv);   /* xml=true dans le contenu texte */
            free(tv);
        }
    }

    if(has_kids)
    {
        j2x_emit_children(out, kids_js);
    }

    nyx_string_builder_append(out, false, false, "</", tag, ">");

    free(tag);
}

/*--------------------------------------------------------------------------------------------------------------------*/
/* API                                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

nyx_j2x_ctx_t *nyx_j2x_init(nyx_j2x_emit_fn emit)
{
    nyx_j2x_ctx_t *ctx = (nyx_j2x_ctx_t *) malloc(sizeof(nyx_j2x_ctx_t));

    if(ctx == NULL) return NULL;

    memset(ctx, 0x00, sizeof(*ctx));

    ctx->emit = emit;

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

    struct mg_str in = { .buf = (char *) text, .len = len };

    /* présence du tag racine requis */
    if(jget(in, "$.<>").len == 0) return;

    nyx_string_builder_t *sb = nyx_string_builder_new();

    j2x_emit_element(sb, in);

    str_t xml = nyx_string_builder_to_string(sb);   /* pas de _cstring dans la nouvelle API */

    if(ctx->emit != NULL && xml != NULL)
    {
        ctx->emit(strlen(xml), xml);
    }

    free(xml);

    nyx_string_builder_free(sb);
}

/*--------------------------------------------------------------------------------------------------------------------*/
