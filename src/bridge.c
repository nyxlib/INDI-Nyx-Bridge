/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include "bridge.h"

#include "external/mongoose.h"

/*--------------------------------------------------------------------------------------------------------------------*/

static struct mg_mgr mgr;

struct mg_connection *indi_connection = NULL;
struct mg_connection *mqtt_connection = NULL;

/*--------------------------------------------------------------------------------------------------------------------*/

static const char *m_indi_url = "";
static const char *m_indi_username = "";
static const char *m_indi_password = "";

static const char *m_mqtt_url = "";
static const char *m_mqtt_username = "";
static const char *m_mqtt_password = "";

/*--------------------------------------------------------------------------------------------------------------------*/

static void indi_handler(struct mg_connection *connection, int ev, void *ev_data)
{
    /**/ if(ev == MG_EV_OPEN)
    {
        MG_INFO(("%lu INDI OPEN", connection->id));

        indi_connection = connection;
    }
    else if(ev == MG_EV_CLOSE)
    {
        MG_INFO(("%lu INDI CLOSE", connection->id));

        indi_connection = NULL;
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
        MG_INFO(("%lu MQTT OPEN", connection->id));

        mqtt_connection = connection;
    }
    else if(ev == MG_EV_CLOSE)
    {
        MG_INFO(("%lu MQTT CLOSE", connection->id));

        mqtt_connection = NULL;
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
    struct mg_mqtt_opts mqtt_opts = {0};

    mqtt_opts.user = mg_str(m_mqtt_username);
    mqtt_opts.pass = mg_str(m_mqtt_password);

    mqtt_opts.client_id = mg_str("$$");

    mqtt_opts.version = 0x04;
    mqtt_opts.clean = true;

    /*----------------------------------------------------------------------------------------------------------------*/
    /* INDI                                                                                                           */
    /*----------------------------------------------------------------------------------------------------------------*/

    if(indi_connection == NULL && m_indi_url != NULL && m_indi_url[0] != '\0')
    {
        indi_connection = mg_connect(
            &mgr,
            m_indi_url,
            indi_handler,
            NULL
        );

        if(indi_connection != NULL)
        {
            MG_INFO(("Connecting to INDI..."));
        }
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    /* MQTT                                                                                                           */
    /*----------------------------------------------------------------------------------------------------------------*/

    if(mqtt_connection == NULL && m_mqtt_url != NULL && m_mqtt_url[0] != '\0')
    {
        mqtt_connection = mg_mqtt_connect(
            &mgr,
            m_mqtt_url,
            &mqtt_opts,
            mqtt_handler,
            NULL
        );

        if(mqtt_connection != NULL)
        {
            MG_INFO(("Connecting to MQTT..."));
        }
    }

}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_initialize()
{
    /*----------------------------------------------------------------------------------------------------------------*/

    mg_log_set(MG_LL_INFO);

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_mgr_init(&mgr);

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_timer_add(&mgr, 1000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, retry_timer_handler, NULL);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_finalize()
{
    mg_mgr_free(&mgr);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_poll(
    const char *indi_url,
    const char *indi_username,
    const char *indi_password,
    /**/
    const char *mqtt_url,
    const char *mqtt_username,
    const char *mqtt_password,
    /**/
    int ms
) {
    /*----------------------------------------------------------------------------------------------------------------*/

    m_indi_url = indi_url;
    m_indi_username = indi_username;
    m_indi_password = indi_password;

    m_mqtt_url = mqtt_url;
    m_mqtt_username = mqtt_username;
    m_mqtt_password = mqtt_password;

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_mgr_poll(&mgr, ms);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/
