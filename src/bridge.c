/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include "bridge.h"

#include "external/mongoose.h"

/*--------------------------------------------------------------------------------------------------------------------*/

static struct mg_mgr m_mgr = {0};

/*--------------------------------------------------------------------------------------------------------------------*/

static char *m_indi_url = NULL;
static char *m_mqtt_url = NULL;
static char *m_mqtt_user = NULL;
static char *m_mqtt_pass = NULL;

static struct mg_mqtt_opts m_mqtt_opts = {0};

/*--------------------------------------------------------------------------------------------------------------------*/

static struct mg_connection *m_indi_connection = NULL;
static struct mg_connection *m_mqtt_connection = NULL;

/*--------------------------------------------------------------------------------------------------------------------*/

static const char *nz(const char *s) { return s ? s : ""; }

static int streq(const char *a, const char *b)
{
    return strcmp(nz(a), nz(b)) == 0;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void indi_handler(struct mg_connection *connection, int ev, void *ev_data)
{
    /**/ if(ev == MG_EV_OPEN)
    {
        MG_INFO(("%lu INDI OPEN (%s)", connection->id, m_indi_url));

        m_indi_connection = connection;
    }
    else if(ev == MG_EV_CLOSE)
    {
        MG_INFO(("%lu INDI CLOSE", connection->id));

        m_indi_connection = NULL;
    }
    else if(ev == MG_EV_ERROR)
    {
        MG_ERROR(("%lu INDI ERROR %s", connection->id, (const char *) ev_data));
    }
    else if(ev == MG_EV_READ)
    {
        /* TODO */
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void mqtt_handler(struct mg_connection *connection, int ev, void *ev_data)
{
    /**/ if(ev == MG_EV_OPEN)
    {
        MG_INFO(("%lu MQTT OPEN (%s)", connection->id, m_mqtt_url));

        m_mqtt_connection = connection;
    }
    else if(ev == MG_EV_CLOSE)
    {
        MG_INFO(("%lu MQTT CLOSE", connection->id));

        m_mqtt_connection = NULL;
    }
    else if(ev == MG_EV_ERROR)
    {
        MG_ERROR(("%lu MQTT ERROR %s", connection->id, (const char *) ev_data));
    }
    else if(ev == MG_EV_MQTT_OPEN)
    {
        MG_INFO(("%lu MQTT OPEN", connection->id));

        /* TODO */
    }
    else if(ev == MG_EV_MQTT_MSG)
    {
        /* TODO */
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void retry_timer_handler(void *arg)
{
    /*----------------------------------------------------------------------------------------------------------------*/
    /* INDI                                                                                                           */
    /*----------------------------------------------------------------------------------------------------------------*/

    if(m_indi_connection == NULL && m_indi_url != NULL && m_indi_url[0] != '\0')
    {
        m_indi_connection = mg_connect(
            &m_mgr,
            m_indi_url,
            indi_handler,
            NULL
        );
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    /* MQTT                                                                                                           */
    /*----------------------------------------------------------------------------------------------------------------*/

    if(m_mqtt_connection == NULL && m_mqtt_url != NULL && m_mqtt_url[0] != '\0')
    {
        m_mqtt_opts.user = mg_str(nz(m_mqtt_user));
        m_mqtt_opts.pass = mg_str(nz(m_mqtt_pass));

        m_mqtt_connection = mg_mqtt_connect(
            &m_mgr,
            m_mqtt_url,
            &m_mqtt_opts,
            mqtt_handler,
            NULL
        );
    }

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_initialize()
{
    /*----------------------------------------------------------------------------------------------------------------*/

    mg_log_set(MG_LL_INFO);

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_mgr_init(&m_mgr);

    /*----------------------------------------------------------------------------------------------------------------*/

    m_mqtt_opts.client_id = mg_str("$$");

    m_mqtt_opts.version = 0x04;
    m_mqtt_opts.clean = true;

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_timer_add(&m_mgr, 1000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, retry_timer_handler, NULL);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_finalize()
{
    mg_mgr_free(&m_mgr);

    free(m_indi_url);
    free(m_mqtt_url);
    free(m_mqtt_user);
    free(m_mqtt_pass);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_poll(
    const char *indi_url,
    const char *mqtt_url,
    const char *mqtt_user,
    const char *mqtt_pass,
    int ms
) {
    /*----------------------------------------------------------------------------------------------------------------*/

    if(!streq(m_indi_url, indi_url))
    {
        free(m_indi_url);

        m_indi_url = strdup(nz(indi_url));

        if(m_indi_connection != NULL)
        {
            //_disconnect(m_indi_connection, NULL);

            m_indi_connection->is_closing = 1;
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    if(!streq(m_mqtt_url, mqtt_url)
       ||
       !streq(m_mqtt_user, mqtt_user)
       ||
       !streq(m_mqtt_pass, mqtt_pass)
    ) {
        free(m_mqtt_url);
        free(m_mqtt_user);
        free(m_mqtt_pass);

        m_mqtt_url = strdup(nz(mqtt_url));
        m_mqtt_user = strdup(nz(mqtt_user));
        m_mqtt_pass = strdup(nz(mqtt_pass));

        if(m_mqtt_connection != NULL)
        {
            mg_mqtt_disconnect(m_mqtt_connection, NULL);

            m_mqtt_connection->is_closing = 1;
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_mgr_poll(&m_mgr, ms);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/
