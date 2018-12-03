/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, Lua MQTT module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_MQTT
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "modules.h"
#include "error.h"
#include "sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <mqtt/MQTTAsync.h>
#include <mqtt/MQTTClientPersistence.h>

#include <sys/mutex.h>
#include <sys/delay.h>
#include <sys/status.h>
#include <sys/syslog.h>
#include <sys/mount.h>

#include <sys/drivers/net.h>

#define MQTT_CONNECT_TIMEOUT 20000

#define evMQTT_CONNECTED  ( 1 << 0 )
#define evMQTT_TIMEOUT    ( 1 << 1 )

void MQTTAsync_init(void);

// Module errors
#define LUA_MQTT_ERR_CANT_CREATE_CLIENT (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  0)
#define LUA_MQTT_ERR_CANT_SET_CALLBACKS (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  1)
#define LUA_MQTT_ERR_CANT_CONNECT       (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  2)
#define LUA_MQTT_ERR_CANT_SUBSCRIBE     (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  3)
#define LUA_MQTT_ERR_CANT_PUBLISH       (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  4)
#define LUA_MQTT_ERR_CANT_DISCONNECT    (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  5)
#define LUA_MQTT_ERR_NOT_ENOUGH_MEMORY  (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  6)

// Register driver and messages
DRIVER_REGISTER_BEGIN(MQTT,mqtt,0,NULL,NULL);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotCreateClient, "can't create client", LUA_MQTT_ERR_CANT_CREATE_CLIENT);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotSetCallbacks, "can't set callbacks", LUA_MQTT_ERR_CANT_SET_CALLBACKS);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotConnect, "can't connect", LUA_MQTT_ERR_CANT_CONNECT);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotSubscribeToTopic, "can't subscribe to topic", LUA_MQTT_ERR_CANT_SUBSCRIBE);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotPublishToTopic, "can't publish to topic", LUA_MQTT_ERR_CANT_PUBLISH);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotDisconnect, "can't disconnect", LUA_MQTT_ERR_CANT_DISCONNECT);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, NotEnoughMemory, "not enough memory", LUA_MQTT_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_END(MQTT,mqtt,0,NULL,NULL);

// MQTT client initialized?
static int initialized = 0;

// MQTT subscription
typedef struct {
    char *topic;              // Subscribed topic
    int qos;                  // QOS
    int subscribed;           // Topic is subscribed to broker?
    lua_callback_t *callback; // Lua callback, called when a message is received on topic
    void *next;               // Next subscribed topic
} mqtt_subs;

// MQTT user data
typedef struct {
    struct mtx mtx;

    TaskHandle_t connTask;
    TaskHandle_t discTask;

    MQTTAsync_connectOptions conn_opts;
#ifdef OPENSSL
    MQTTAsync_SSLOptions ssl_opts;
    const char *ca_file;
#endif
    MQTTAsync client;

    // Subscription list
    mqtt_subs *subs;

    int secure;
    int persistence;
} mqtt_userdata;

// Emit a Lua exception using the result code (rc) provided by the MQTT library
static int mqtt_emit_exeption(lua_State* L, int exception, int rc) {
    switch (rc) {
    case MQTTASYNC_FAILURE:
        return luaL_exception_extended(L, exception, "client failure");
        break;

    case MQTTASYNC_PERSISTENCE_ERROR:
        return luaL_exception_extended(L, exception, "persistence error");
        break;

    case MQTTASYNC_DISCONNECTED:
        return luaL_exception_extended(L, exception, "client is disconnected");
        break;

    case MQTTASYNC_MAX_MESSAGES_INFLIGHT:
        return luaL_exception_extended(L, exception, "maximum number of messages allowed to be simultaneously in-flight has been reached");
        break;

    case MQTTASYNC_BAD_UTF8_STRING:
        return luaL_exception_extended(L, exception, "an invalid UTF-8 string has been detected");
        break;

    case MQTTASYNC_NULL_PARAMETER:
        return luaL_exception_extended(L, exception, "a NULL parameter has been supplied when this is invalid");
        break;

    case MQTTASYNC_TOPICNAME_TRUNCATED:
        return luaL_exception_extended(L, exception, "the topic has been truncated (the topic string includes embedded NULL characters");
        break;

    case MQTTASYNC_BAD_STRUCTURE:
        return luaL_exception_extended(L, exception, "a structure parameter does not have the correct eye-catcher and version number");
        break;

    case MQTTASYNC_BAD_QOS:
        return luaL_exception_extended(L, exception, "a qos parameter is not 0, 1 or 2");
        break;

    case MQTTASYNC_NO_MORE_MSGIDS:
        return luaL_exception_extended(L, exception, "all message ids are being used");
        break;

    case MQTTASYNC_OPERATION_INCOMPLETE:
        return luaL_exception_extended(L, exception, "the request is being discarded when not complete");
        break;

    case MQTTASYNC_MAX_BUFFERED_MESSAGES:
        return luaL_exception_extended(L, exception, "no more messages can be buffered");
        break;

    case MQTTASYNC_SSL_NOT_SUPPORTED:
        return luaL_exception_extended(L, exception, "attempting SSL connection using non-SSL version of library");
        break;

    case MQTTASYNC_BAD_PROTOCOL:
        return luaL_exception_extended(L, exception, "protocol prefix in serverURI should be tcp:// or ssl://");
        break;
    }

    return 0;
}

// Extracted from:
//
// http://git.eclipse.org/c/mosquitto/org.eclipse.mosquitto.git/tree/lib/util_mosq.c
//
// mosquitto_topic_matches_sub function
static int topic_matches_sub(const char *sub, const char *topic) {
    int slen, tlen;
    int spos, tpos;
    bool multilevel_wildcard = false;

    slen = strlen(sub);
    tlen = strlen(topic);

    if (slen && tlen) {
        if ((sub[0] == '$' && topic[0] != '$')
                || (topic[0] == '$' && sub[0] != '$')) {

            return 0;
        }
    }

    spos = 0;
    tpos = 0;

    while (spos < slen && tpos < tlen) {
        if (sub[spos] == topic[tpos]) {
            if (tpos == tlen - 1) {
                /* Check for e.g. foo matching foo/# */
                if (spos == slen - 3 && sub[spos + 1] == '/'
                        && sub[spos + 2] == '#') {
                    multilevel_wildcard = true;
                    return 1;
                }
            }
            spos++;
            tpos++;
            if (spos == slen && tpos == tlen) {
                return 1;
            } else if (tpos == tlen && spos == slen - 1 && sub[spos] == '+') {
                spos++;
                return 1;
            }
        } else {
            if (sub[spos] == '+') {
                spos++;
                while (tpos < tlen && topic[tpos] != '/') {
                    tpos++;
                }
                if (tpos == tlen && spos == slen) {
                    return 1;
                }
            } else if (sub[spos] == '#') {
                multilevel_wildcard = true;
                if (spos + 1 != slen) {
                    return 0;
                } else {
                    return 1;
                }
            } else {
                return 0;
            }
        }
    }

    if (multilevel_wildcard == false && (tpos < tlen || spos < slen)) {
        return 0;
    }

    return 0;
}

// Add a topic to the subscription list, and subscribe the topic to the broker if client is
// connected. Also, a Lua callback is linked with the topic.
static int add_subs(lua_State *L, int index, mqtt_userdata *mqtt, const char *topic, int qos) {
    // Create and populate subscription structure
    mqtt_subs *subs = (mqtt_subs *)calloc(1, sizeof(mqtt_subs));
    if (!subs) {
        return luaL_exception_extended(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Copy topic & QOS
    subs->topic = strdup(topic);
    subs->qos = qos;

    // Not subscribed yet
    subs->subscribed = 0;

    if (!subs->topic) {
        free(subs);
        return luaL_exception_extended(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Create the lua callback
    subs->callback = luaS_callback_create(L, index);
    if (subs->callback == NULL) {
        free(subs->topic);
        free(subs);

        return luaL_exception_extended(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Add the subscription to the subscription list
    subs->next = mqtt->subs;
    mqtt->subs = subs;

    if (MQTTAsync_isConnected(mqtt->client)) {
    		// If client is connected, subscribe to topic now
    		int rc;

		if ((rc = MQTTAsync_subscribe(mqtt->client, topic, qos, NULL)) != MQTTASYNC_SUCCESS) {
			return rc;
		} else {
			subs->subscribed = 1;
		}
    }

    return 0;
}

// Subscribe all pending topics to the broker
static int subs_subscribe(mqtt_userdata *mqtt) {
    mqtt_subs *subs;
    int rc = 0;

    mtx_lock(&mqtt->mtx);

    subs = mqtt->subs;
    while (subs) {
    		if (!subs->subscribed) {
			if ((rc = MQTTAsync_subscribe(mqtt->client, subs->topic, subs->qos, NULL)) == MQTTASYNC_SUCCESS) {
				subs->subscribed = 1;
			} else {
			    mtx_unlock(&mqtt->mtx);
				return rc;
			}
    		}

    		subs = subs->next;
    }

    mtx_unlock(&mqtt->mtx);

    return rc;
}

// Connection failure callback, called by the MQTT client when the connection can't
// be established.
static void connFailure(void *context, MQTTAsync_failureData *response) {
    mqtt_userdata *mqtt = (mqtt_userdata *)context;

    if (response) {
            // If a task is waiting for the connection, notify the task with the
            // response code
        mtx_lock(&mqtt->mtx);
        if (mqtt->connTask) {
            xTaskNotify(mqtt->connTask, response->code, eSetValueWithOverwrite);
        }
        mtx_unlock(&mqtt->mtx);
    }
}

// Connection success callback, called by he MQTT client when the connection is
// established to the MQTT broker.
static void connSuccess(void *context, MQTTAsync_successData *response) {
    mqtt_userdata *mqtt = (mqtt_userdata *)context;

	// Subscribe to topics
	int rc;

	if ((rc = subs_subscribe(mqtt)) != 0) {
		// If a task is waiting for the connection, notify the task with the
		// response code
		mtx_lock(&mqtt->mtx);
		if (mqtt->connTask) {
			xTaskNotify(mqtt->connTask, rc, eSetValueWithOverwrite);
		}
		mtx_unlock(&mqtt->mtx);
	} else {
		// If a task is waiting for the connection, notify the task with the
		// response code
		mtx_lock(&mqtt->mtx);
		if (mqtt->connTask) {
			xTaskNotify(mqtt->connTask, 0, eSetValueWithOverwrite);
		}
		mtx_unlock(&mqtt->mtx);
	}
}

// Disconnection failure callback, called by the MQTT client when there is
// a problem with the disconnection.
static void discFailure(void *context, MQTTAsync_failureData *response) {
    mqtt_userdata *mqtt = (mqtt_userdata *)context;

    if (response) {
        // If a task is waiting for the disconnection, notify the task with the
        // response code
        mtx_lock(&mqtt->mtx);
        if (mqtt->discTask) {
            xTaskNotify(mqtt->discTask, response->code, eSetValueWithOverwrite);
        }
        mtx_unlock(&mqtt->mtx);
    }
}

// Disconnection success callback, called by he MQTT client when the disconnection
// is done.
static void discSuccess(void *context, MQTTAsync_successData *response) {
    mqtt_userdata *mqtt = (mqtt_userdata *)context;

	// If a task is waiting for the disconnection, notify the task with the
	// response code
	mtx_lock(&mqtt->mtx);
	if (mqtt->discTask) {
		xTaskNotify(mqtt->discTask, 0, eSetValueWithOverwrite);
	}
	mtx_unlock(&mqtt->mtx);
}

// Message arrived callback
static int msgArrived(void *context, char * topicName, int topicLen, MQTTAsync_message* m) {
    mqtt_userdata *mqtt = (mqtt_userdata *) context;
    if (mqtt) {
        mqtt_subs *subs;

        mtx_lock(&mqtt->mtx);

        subs = mqtt->subs;
        while (subs) {
            if (topic_matches_sub(subs->topic, topicName)) {
                // Push argument for the callback's function
                lua_pushinteger(luaS_callback_state(subs->callback), m->payloadlen);
                lua_pushlstring(luaS_callback_state(subs->callback), m->payload, m->payloadlen);

                // see: https://www.ibm.com/support/knowledgecenter/SSFKSJ_7.5.0/com.ibm.mq.javadoc.doc/WMQMQxrCClasses/_m_q_t_t_client_8h.html?view=kc#aa42130dd069e7e949bcab37b6dce64a5
                if (topicLen == 0) {
                    lua_pushinteger(luaS_callback_state(subs->callback), strlen(topicName));
                    lua_pushstring(luaS_callback_state(subs->callback), topicName);
                } else {
                    lua_pushinteger(luaS_callback_state(subs->callback), topicLen);
                    lua_pushlstring(luaS_callback_state(subs->callback), topicName, topicLen);
                }

                luaS_callback_call(subs->callback, 4);
            }
            subs = subs->next;
        }

        MQTTAsync_freeMessage(&m);
        MQTTAsync_free(topicName);

        mtx_unlock(&mqtt->mtx);
    }

    return 1;
}

static int lmqtt_client(lua_State* L) {
    int rc = 0;
    size_t lenClientId, lenHost;
    char url[250];
    const char *persistence_folder = NULL;
    int persistence = MQTTCLIENT_PERSISTENCE_NONE;

    const char *clientId = luaL_checklstring(L, 1, &lenClientId); //is being strdup'd in MQTTClient_create
    const char *host = luaL_checklstring(L, 2, &lenHost); //url is being strdup'd in MQTTClient_create
    int port = luaL_checkinteger(L, 3);

    luaL_checktype(L, 4, LUA_TBOOLEAN);
    int secure = lua_toboolean(L, 4);

#ifdef OPENSSL
    const char *ca_file = luaL_optstring(L, 5, NULL); //is being strdup'd below
#endif

    if (lua_gettop(L) > 5) {
        luaL_checktype(L, 6, LUA_TBOOLEAN);
        persistence =
                lua_toboolean(L, 6) ?
                        MQTTCLIENT_PERSISTENCE_DEFAULT :
                        MQTTCLIENT_PERSISTENCE_NONE;
        persistence_folder = luaL_optstring(L, 7, NULL); //is being strdup'd in MQTTClient_create
    }

    // Allocate mqtt structure and initialize
    mqtt_userdata * mqtt = (mqtt_userdata *) lua_newuserdata(L, sizeof(mqtt_userdata));
    mqtt->connTask = NULL;
    mqtt->discTask = NULL;
    mqtt->client = NULL;
    mqtt->subs = NULL;
    mqtt->secure = secure;
    mqtt->persistence = persistence;
#ifdef OPENSSL
    mqtt->ca_file = (ca_file ? strdup(ca_file) : NULL); //save for use during mqtt_connect
#endif
    mtx_init(&mqtt->mtx, NULL, NULL, 0);

    // needed for lmqtt_client_gc to not crash freeing the username and password
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

    bcopy(&conn_opts, &mqtt->conn_opts, sizeof(MQTTAsync_connectOptions));

    // Calculate uri
    snprintf(url, sizeof(url), "%s://%s:%d", (mqtt->secure?"ssl":"tcp"), host, port);

    if (!initialized) {
        MQTTAsync_init();
        initialized = 1;
    }

    //url is being strdup'd in MQTTClient_create
    MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;

    create_opts.sendWhileDisconnected = (persistence == MQTTCLIENT_PERSISTENCE_DEFAULT);

    rc = MQTTAsync_createWithOptions(&mqtt->client, url, clientId, persistence, (char*)persistence_folder, &create_opts);
    if (rc < 0) {
        return mqtt_emit_exeption(L, LUA_MQTT_ERR_CANT_CREATE_CLIENT, rc);
    }

    rc = MQTTAsync_setCallbacks(mqtt->client, mqtt, NULL, msgArrived, NULL);
    if (rc < 0) {
        return mqtt_emit_exeption(L, LUA_MQTT_ERR_CANT_SET_CALLBACKS, rc);
    }

    luaL_getmetatable(L, "mqtt.cli");
    lua_setmetatable(L, -2);

    return 1;
}

static int lmqtt_connected(lua_State* L) {
    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_checkudata(L, 1, "mqtt.cli");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");

    lua_pushboolean(L, ((MQTTAsync_isConnected(mqtt->client) == 1) ?1:0));

    return 1;
}

static int lmqtt_connect(lua_State* L) {
    const char *user;
    const char *password;

    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_checkudata(L, 1, "mqtt.cli");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");

    // Get arguments
    user = luaL_checkstring(L, 2);
    password = luaL_checkstring(L, 3);

    int clean = 0;
    if (lua_gettop(L) >= 4) {
        luaL_checktype(L, 4, LUA_TBOOLEAN);
        clean = lua_toboolean(L, 4);
    }


#ifdef OPENSSL
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    ssl_opts.trustStore = mqtt->ca_file; //has been strdup'd already
    ssl_opts.enableServerCertAuth = (ssl_opts.trustStore != NULL);
    bcopy(&ssl_opts, &mqtt->ssl_opts, sizeof(MQTTAsync_SSLOptions));
#endif

    // Set connection context
    mqtt->connTask = xTaskGetCurrentTaskHandle();

    // Prepare connection
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.connectTimeout = MQTT_CONNECT_TIMEOUT;
    conn_opts.cleansession = clean;
    conn_opts.keepAliveInterval = 20;

    if (strlen(user) > 0) {
        conn_opts.username = strdup(user); //needs to be strdup'd here for connectionLost usage
    }

    if (strlen(password) > 0) {
        conn_opts.password = strdup(password); // //needs to be strdup'd here for connectionLost usage
    }

    conn_opts.onSuccess = connSuccess;
    conn_opts.onFailure = connFailure;
    conn_opts.automaticReconnect = 1;
    conn_opts.maxRetryInterval = 10;

    conn_opts.context = mqtt;
#ifdef OPENSSL
    conn_opts.ssl = &mqtt->ssl_opts;
#endif

    //if calling connect() twice in a row, make sure the subscription callbacks are properly free'd mqtt_subs *callback;
#if 0
    mqtt_subs *callback;
    mqtt_subs *nextcallback;

    callback = mqtt->subs;
    while (callback) {
        luaS_callback_destroy(callback->callback);

        nextcallback = callback->next;
        if (callback->topic){
          free(callback->topic);
          callback->topic = NULL;
        }

        free(callback);
        callback = nextcallback;
    }
    mqtt->subs = NULL;
#endif

    //if calling connect() twice in a row, make sure user and password are properly free'd
    if (mqtt->conn_opts.username) {
        free((char*) mqtt->conn_opts.username);
        mqtt->conn_opts.username = NULL;
    }

    if (mqtt->conn_opts.password) {
        free((char*) mqtt->conn_opts.password);
        mqtt->conn_opts.password = NULL;
    }

    bcopy(&conn_opts, &mqtt->conn_opts, sizeof(MQTTAsync_connectOptions));

    // Try to connect
    if (!wait_for_network_init(10)) {
        mtx_lock(&mqtt->mtx);
        mqtt->connTask = NULL;
        mtx_unlock(&mqtt->mtx);

    		return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "network not started");
    }

    MQTTAsync_connect(mqtt->client, &mqtt->conn_opts);

    // Wait for connection
    uint32_t rc_val;
    if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &rc_val, MQTT_CONNECT_TIMEOUT / portTICK_PERIOD_MS) == pdTRUE) {
        mtx_lock(&mqtt->mtx);
        mqtt->connTask = NULL;
        mtx_unlock(&mqtt->mtx);

        switch (rc_val) {
            case 1: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, unacceptable protocol version");
            case 2: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, identifier rejected");
            case 3: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, server unavailable");
            case 4: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, bad username or password");
            case 5: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, not authorized");
        }
    } else {
        mtx_lock(&mqtt->mtx);
        mqtt->connTask = NULL;
        mtx_unlock(&mqtt->mtx);

        return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "timeout");
    }

    mtx_lock(&mqtt->mtx);
    mqtt->connTask = NULL;
    mtx_unlock(&mqtt->mtx);

    return 0;
}

static int lmqtt_subscribe(lua_State* L) {
    int rc;
    int qos;
    const char *topic;

    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_checkudata(L, 1, "mqtt.cli");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");

    topic = luaL_checkstring(L, 2);
    qos = luaL_checkinteger(L, 3);

    if ((qos > 0) && (mqtt->persistence == MQTTCLIENT_PERSISTENCE_NONE)) {
        return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_SUBSCRIBE, "enable persistence for a qos > 0");
    }

    // Add subscription
    mtx_lock(&mqtt->mtx);
    if ((rc = add_subs(L, 4, mqtt, topic, qos)) != 0) {
    		return mqtt_emit_exeption(L, LUA_MQTT_ERR_CANT_SUBSCRIBE, rc);
    }
    mtx_unlock(&mqtt->mtx);

    return 0;
}

static int lmqtt_publish(lua_State* L) {
    int rc;
    int qos;
    size_t payload_len;
    const char *topic;
    char *payload;
    int retained = 0;

    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_checkudata(L, 1, "mqtt.cli");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");

    // Get arguments
    topic = luaL_checkstring(L, 2);
    payload = (char *) luaL_checklstring(L, 3, &payload_len);
    qos = luaL_checkinteger(L, 4);

    // Sanity checks
    if (qos > 0 && mqtt->persistence == MQTTCLIENT_PERSISTENCE_NONE) {
        return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_PUBLISH, "enable persistence for a qos > 0");
    }

    if (lua_gettop(L) >= 5) {
        luaL_checktype(L, 5, LUA_TBOOLEAN);
        retained = lua_toboolean(L, 5);
    }

    // Prepare message
    MQTTAsync_message msg = MQTTAsync_message_initializer;

    msg.payload = payload;
    msg.payloadlen = strlen(payload);
    msg.qos = qos;
    msg.retained = retained;

    // Send message
    if ((rc = MQTTAsync_sendMessage(mqtt->client, topic, &msg, NULL)) != MQTTASYNC_SUCCESS) {
        return mqtt_emit_exeption(L, LUA_MQTT_ERR_CANT_PUBLISH, rc);
    }

    return 0;
}

static int lmqtt_disconnect(lua_State* L) {
    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_checkudata(L, 1, "mqtt.cli");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");

    // Set connection context
    mtx_lock(&mqtt->mtx);
    mqtt->discTask = xTaskGetCurrentTaskHandle();
    mtx_unlock(&mqtt->mtx);

    // Prepare disconnection
    MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
    opts.onSuccess = discSuccess;
    opts.onFailure = discFailure;
    opts.context = mqtt;

    // Try to disconnect

    MQTTAsync_disconnect(mqtt->client, &opts);

    // Wait for disconnection
    uint32_t rc_val;

    if (MQTTAsync_isConnected(mqtt->client)) {
        if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &rc_val, MQTT_CONNECT_TIMEOUT / portTICK_PERIOD_MS) == pdFALSE) {
            mtx_lock(&mqtt->mtx);
            mqtt->discTask = NULL;
            mtx_unlock(&mqtt->mtx);

            return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_DISCONNECT, "timeout");
        }
    }

    mtx_lock(&mqtt->mtx);
    mqtt->discTask = NULL;

    // Mark all subscribed topics as not subscribed to the broker. We don't destroy anything related to
    // topics when disconnect.
    mqtt_subs *subs;

    subs = mqtt->subs;
    while (subs) {
    		subs->subscribed = 0;
    		subs = subs->next;
    }

    //make sure user and password are properly free'd
    if (mqtt->conn_opts.username) {
        free((char*) mqtt->conn_opts.username);
        mqtt->conn_opts.username = NULL;
    }

    if (mqtt->conn_opts.password) {
        free((char*) mqtt->conn_opts.password);
        mqtt->conn_opts.password = NULL;
    }

    mtx_unlock(&mqtt->mtx);

    return 0;
}

// Destructor
static int lmqtt_client_gc(lua_State *L) {
    mqtt_subs *subs;
    mqtt_subs *next_subs;

    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_testudata(L, 1, "mqtt.cli");
    if (mqtt) {
        mtx_lock(&mqtt->mtx);

        // Free all the resources used by the subscribed topics
        subs = mqtt->subs;
        while (subs) {
            luaS_callback_destroy(subs->callback);

            next_subs = subs->next;
            if (subs->topic){
              free(subs->topic);
              subs->topic = NULL;
            }

            free(subs);
            subs = next_subs;
        }

        mqtt->subs = NULL;

        // Destroy client
        MQTTAsync_destroy(&mqtt->client);
        mqtt->client = NULL;

#ifdef OPENSSL
        if (mqtt->ca_file) {
            free((char*) mqtt->ca_file);
            mqtt->ca_file = NULL;
            mqtt->ssl_opts.trustStore = NULL;
        }
#endif

        if (mqtt->conn_opts.username) {
            free((char*) mqtt->conn_opts.username);
            mqtt->conn_opts.username = NULL;
        }

        if (mqtt->conn_opts.password) {
            free((char*) mqtt->conn_opts.password);
            mqtt->conn_opts.password = NULL;
        }

        mtx_unlock(&mqtt->mtx);
        mtx_destroy(&mqtt->mtx);
    }

    return 0;
}

static const LUA_REG_TYPE lmqtt_map[] = {
    { LSTRKEY( "client"      ),   LFUNCVAL( lmqtt_client     ) },

    { LSTRKEY("QOS0"), LINTVAL(0) },
    { LSTRKEY("QOS1"), LINTVAL(1) },
    { LSTRKEY("QOS2"), LINTVAL(2) },

    { LSTRKEY("PERSISTENCE_FILE"), LINTVAL(MQTTCLIENT_PERSISTENCE_DEFAULT) },
    { LSTRKEY("PERSISTENCE_NONE"), LINTVAL(MQTTCLIENT_PERSISTENCE_NONE) },
    { LSTRKEY("PERSISTENCE_USER"), LINTVAL(MQTTCLIENT_PERSISTENCE_USER) },

    // Error definitions
    DRIVER_REGISTER_LUA_ERRORS(mqtt)
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lmqtt_client_map[] = {
    { LSTRKEY( "connect"     ),   LFUNCVAL( lmqtt_connect    ) },
    { LSTRKEY( "connected"   ),   LFUNCVAL( lmqtt_connected  ) },
    { LSTRKEY( "disconnect"  ),   LFUNCVAL( lmqtt_disconnect ) },
    { LSTRKEY( "subscribe"   ),   LFUNCVAL( lmqtt_subscribe  ) },
    { LSTRKEY( "publish"     ),   LFUNCVAL( lmqtt_publish    ) },
    { LSTRKEY( "__metatable" ),   LROVAL  ( lmqtt_client_map ) },
    { LSTRKEY( "__index"     ),   LROVAL  ( lmqtt_client_map ) },
    { LSTRKEY( "__gc"        ),   LFUNCVAL( lmqtt_client_gc  ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_mqtt( lua_State *L ) {
    luaL_newmetarotable(L,"mqtt.cli", (void *)lmqtt_client_map);

#if !LUA_USE_ROTABLE
    luaL_newlib(L, mqtt);
    return 1;
#else
    return 0;
#endif
}

MODULE_REGISTER_ROM(MQTT, mqtt, lmqtt_map, luaopen_mqtt, 1);

#endif
