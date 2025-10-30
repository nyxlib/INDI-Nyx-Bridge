/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include "bridge.h"

#include "external/mongoose.h"

/*--------------------------------------------------------------------------------------------------------------------*/

#define MQTT_TOPIC_PING "nyx/ping/node"

#define MQTT_TOPIC_IN "nyx/cmd/json"

#define MQTT_TOPIC_OUT "nyx/json"

/*--------------------------------------------------------------------------------------------------------------------*/

static struct mg_mgr m_mgr = {0};

/*--------------------------------------------------------------------------------------------------------------------*/

static STR_t m_indi_url = NULL;
static str_t m_mqtt_url = NULL;
static str_t m_mqtt_user = NULL;
static str_t m_mqtt_pass = NULL;

static struct mg_mqtt_opts m_mqtt_opts = {0};

/*--------------------------------------------------------------------------------------------------------------------*/

static struct mg_connection *m_indi_connection = NULL;
static struct mg_connection *m_mqtt_connection = NULL;


/*--------------------------------------------------------------------------------------------------------------------*/

static nyx_x2j_ctx_t *m_x2j = NULL;
static nyx_j2x_ctx_t *m_j2x = NULL;

/*--------------------------------------------------------------------------------------------------------------------*/

static bool refresh = true;

/*--------------------------------------------------------------------------------------------------------------------*/

static STR_t nz(STR_t s) { return s != NULL ? s : ""; }

/*--------------------------------------------------------------------------------------------------------------------*/

static bool streq(STR_t a, STR_t b)
{
    return strcmp(nz(a), nz(b)) == 0;
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void json_emit(size_t len, STR_t json)
{
    if(m_mqtt_connection != NULL && len > 0 && json != NULL)
    {
        struct mg_mqtt_opts opts = {
            .topic = mg_str(MQTT_TOPIC_OUT),
            .message = mg_str_n(json, len),
            .qos = 2,
        };

        mg_mqtt_pub(m_mqtt_connection, &opts);

        MG_DEBUG(("%s", json));
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void xml_emit(size_t len, STR_t xml)
{
    if(m_indi_connection != NULL && len > 0 && xml != NULL)
    {
        mg_send(m_indi_connection, xml, len);

        MG_DEBUG(("%s", xml));
    }
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
        MG_ERROR(("%lu INDI ERROR %s", connection->id, (STR_t) ev_data));
    }
    else if(ev == MG_EV_CONNECT)
    {
        refresh = true;
    }
    else if(ev == MG_EV_READ)
    {
        if(connection->recv.len > 0)
        {
            MG_DEBUG(("%.*s", connection->recv.len, (STR_t) connection->recv.buf));

            nyx_x2j_feed(m_x2j, connection->recv.len, (STR_t) connection->recv.buf);

            mg_iobuf_del(&connection->recv, 0, connection->recv.len);
        }
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
        MG_ERROR(("%lu MQTT ERROR %s", connection->id, (STR_t) ev_data));
    }
    else if(ev == MG_EV_MQTT_OPEN)
    {
        struct mg_mqtt_opts opts = {
            .topic = mg_str(MQTT_TOPIC_IN),
            .qos = 2,
        };

        mg_mqtt_sub(connection, &opts);
    }
    else if(ev == MG_EV_MQTT_MSG)
    {
        struct mg_mqtt_message *message = ev_data;

        if(message->data.len > 0)
        {
            MG_DEBUG(("%.*s", message->data.len, (STR_t) message->data.buf));

            nyx_j2x_feed(m_j2x, message->data.len, message->data.buf);
        }
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/

static void ping_handler(void *arg)
{
    if(m_mqtt_connection != NULL)
    {
        struct mg_mqtt_opts opts = {
            .topic = mg_str(MQTT_TOPIC_PING),
            .message = mg_str(NYX_BRIDGE_NAME),
            .qos = 0,
        };

        mg_mqtt_pub(m_mqtt_connection, &opts);
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
    /* GET PROPERTIES                                                                                                 */
    /*----------------------------------------------------------------------------------------------------------------*/

    if(refresh)
    {
        /*------------------------------------------------------------------------------------------------------------*/

        STR_t message1 = "{\"<>\":\"delINDI\"}";

        json_emit(strlen(message1), message1);

        /*------------------------------------------------------------------------------------------------------------*/

        STR_t message2 = "<getProperties version=\"1.7\"/>";

        xml_emit(strlen(message2), message2);

        /*------------------------------------------------------------------------------------------------------------*/

        refresh = false;

        /*------------------------------------------------------------------------------------------------------------*/
    }

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_initialize()
{
    /*----------------------------------------------------------------------------------------------------------------*/

    STR_t env = getenv("NYX_VERBOSE");

    mg_log_set(env != NULL && strcmp(env, "1") == 0 ? MG_LL_DEBUG
                                                    : MG_LL_INFO
    );

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_mgr_init(&m_mgr);

    /*----------------------------------------------------------------------------------------------------------------*/

    m_mqtt_opts.client_id = mg_str(NYX_BRIDGE_NAME);

    m_mqtt_opts.version = 0x04;
    m_mqtt_opts.clean = true;

    /*----------------------------------------------------------------------------------------------------------------*/

    m_x2j = nyx_x2j_init(json_emit);
    m_j2x = nyx_j2x_init(xml_emit);

    /*----------------------------------------------------------------------------------------------------------------*/

    m_indi_url = nyx_discover_indi_url();

    /*----------------------------------------------------------------------------------------------------------------*/

    mg_timer_add(&m_mgr, 5000UL, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, ping_handler, NULL);

    mg_timer_add(&m_mgr, 2000UL, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, retry_timer_handler, NULL);

    /*----------------------------------------------------------------------------------------------------------------*/
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_finalize()
{
    mg_mgr_free(&m_mgr);

    nyx_x2j_close(m_x2j);
    nyx_j2x_close(m_j2x);

    free(m_mqtt_url);
    free(m_mqtt_user);
    free(m_mqtt_pass);
}

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_bridge_poll(
    STR_t mqtt_url,
    STR_t mqtt_user,
    STR_t mqtt_pass,
    int ms
) {
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
