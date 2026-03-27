
#include <PubSubClient.h>
#include "cell_mqtt.h"
#include "cell_cmd.h"


typedef struct {
	struct
	{
		String	topic;
		void	(*callback)(String topic, String data, int len);
	} sub[CONFIG_CELL_MQTT_MAX_SUB_TOPIC];
	uint8_t	count;
} sub_topic_t;

CONFIG_LOG_TAG("LTE_MQTT");
static cell_mqtt_cfg_t g_mqtt_cfg;
static sub_topic_t g_mqtt_sub_topic;
static int g_mqtt_error_cnt = 0;
static boolean g_mqtt_isConnected = false;
static long g_mqtt_last_ms = 0;

extern TinyGsm modem;
static TinyGsmClientSecure net_ssl(modem);
static PubSubClient mqtt(net_ssl);


static void cell_mqtt_rx_callback(char *topic, byte *payload, unsigned int length);
static bool cell_mqtt_set_subscribe_topic(char *topic, void callback(String, String, int));


void cell_mqtt_init(char *clientId, uint16_t keepAlive)
{
    g_mqtt_error_cnt = 0;
    g_mqtt_cfg.clientId = clientId;
    mqtt.setCallback(cell_mqtt_rx_callback);
    mqtt.setKeepAlive(keepAlive);
    mqtt.setBufferSize(CONFIG_CELL_MQTT_BUF_SIZE);
}


void lte_mqtt_set_server( char *host, int port )
{
    mqtt.setServer(host, port);    // MQTT Broker setup
}


void lte_mqtt_set_credentials( char *user, char *password )
{
    g_mqtt_cfg.user = user;
    g_mqtt_cfg.password = password;
}


bool lte_mqtt_connect(void)
{
    FUNC_CELL_LOCK();
    DBG_OUT("connecting to broker \"%s\" port %d user \"%s\" password \"%s\"\r\n", g_mqtt_cfg.url, g_mqtt_cfg.port, g_mqtt_cfg.user, g_mqtt_cfg.password);
    g_mqtt_error_cnt = 0;
    g_mqtt_isConnected = false;

    if( g_mqtt_cfg.clientId == NULL ) {
        DBG_OUT_E("failed. clientId is NULL\r\n");
        FUNC_CELL_UNLOCK();
        return false;
    }

    if( g_mqtt_cfg.url == NULL ) {
        DBG_OUT_E("failed. URL is NULL\r\n");
        FUNC_CELL_UNLOCK();
        return false;
    }

    mqtt.disconnect();

    if( mqtt.connect(g_mqtt_cfg.clientId, g_mqtt_cfg.user, g_mqtt_cfg.password) == false ) {
        DBG_OUT_E("failed to connect to broker\r\n");
        FUNC_CELL_UNLOCK();
        return false;
    }

    g_mqtt_error_cnt = 0;
    DBG_OUT("mqtt connected\r\n");
    FUNC_CELL_UNLOCK();
    return mqtt.connected();
}


void lte_mqtt_disconnect( void )
{
    FUNC_CELL_LOCK();
    mqtt.disconnect();
    FUNC_CELL_UNLOCK();
}