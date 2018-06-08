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
#define LUA_MQTT_ERR_LOST_CONNECTION    (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  6)
#define LUA_MQTT_ERR_NOT_ENOUGH_MEMORY  (DRIVER_EXCEPTION_BASE(MQTT_DRIVER_ID) |  7)

// Register driver and messages
DRIVER_REGISTER_BEGIN(MQTT,mqtt,0,NULL,NULL);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotCreateClient, "can't create client", LUA_MQTT_ERR_CANT_CREATE_CLIENT);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotSetCallbacks, "can't set callbacks", LUA_MQTT_ERR_CANT_SET_CALLBACKS);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotConnect, "can't connect", LUA_MQTT_ERR_CANT_CONNECT);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotSubscribeToTopic, "can't subscribe to topic", LUA_MQTT_ERR_CANT_SUBSCRIBE);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotPublishToTopic, "can't publish to topic", LUA_MQTT_ERR_CANT_PUBLISH);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, CannotDisconnect, "can't disconnect", LUA_MQTT_ERR_CANT_DISCONNECT);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, LostConnection, "lost connection", LUA_MQTT_ERR_LOST_CONNECTION);
    DRIVER_REGISTER_ERROR(MQTT, mqtt, NotEnoughMemory, "not enough memory", LUA_MQTT_ERR_NOT_ENOUGH_MEMORY);
DRIVER_REGISTER_END(MQTT,mqtt,0,NULL,NULL);

static int client_inited = 0;

// MQTT subscription callback
typedef struct {
    char *topic;              // Topic
    int qos;                  // QOS
    lua_callback_t *callback; // Lua callback
    void *next;               // Next callback
} mqtt_subs_callback;

// MQTT user data
typedef struct {
    struct mtx mtx;

    MQTTAsync_connectOptions conn_opts;
#ifdef OPENSSL
    MQTTAsync_SSLOptions ssl_opts;
    const char *ca_file;
#endif
    MQTTAsync client;

    mqtt_subs_callback *callbacks;

    int secure;
    int persistence;
} mqtt_userdata;

// MQTT connection context
typedef struct {
    mqtt_userdata *mqtt;
    TaskHandle_t task;
} mqtt_conn_ctx_t;

// MQTT subscription context
typedef struct {
    mqtt_userdata *mqtt;
    char *topic;
    int qos;
} mqtt_subs_ctx_t;

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

// Connection failure callback, called by the MQTT client when the connection can't
// be established.
static void connFailure(void *context, MQTTAsync_failureData *response) {
    return;

    mqtt_conn_ctx_t *ctx = (mqtt_conn_ctx_t *)context;

    if (response) {
        if (ctx->task) {
            xTaskNotify(ctx->task, response->code, eSetValueWithOverwrite);
        }
    }
}

// Connection success callback, called by he MQTT client when the connection is
// established to the MQTT broker.
static void connSuccess(void *context, MQTTAsync_successData *response) {
    mqtt_conn_ctx_t *ctx = (mqtt_conn_ctx_t *)context;

    if (ctx->task) {
        xTaskNotify(ctx->task, 0, eSetValueWithOverwrite);
    }
}

// Disconnection failure callback, called by the MQTT client when there is
// a problem with the disconnection.
static void discFailure(void *context, MQTTAsync_failureData *response) {
    mqtt_conn_ctx_t *ctx = (mqtt_conn_ctx_t *)context;

    if (response) {
        if (ctx->task) {
            xTaskNotify(ctx->task, response->code, eSetValueWithOverwrite);
        }
    }
}

// Disconnection success callback, called by he MQTT client when the disconnection
// is done.
static void discSuccess(void *context, MQTTAsync_successData *response) {
    mqtt_conn_ctx_t *ctx = (mqtt_conn_ctx_t *)context;

    if (ctx->task) {
        xTaskNotify(ctx->task, 0, eSetValueWithOverwrite);
    }
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

static int add_subs_callback(lua_State *L, int index, mqtt_userdata *mqtt, const char *topic, int qos) {
    // Create and populate callback structure
    mqtt_subs_callback *callback = (mqtt_subs_callback *)calloc(1, sizeof(mqtt_subs_callback));
    if (!callback) {
        return luaL_exception_extended(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    callback->topic = strdup(topic);
    callback->qos = qos;

    if (!callback->topic) {
        free(callback);
        return luaL_exception_extended(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    callback->callback = luaS_callback_create(L, index);
    if (callback->callback == NULL) {
        free(callback->topic);
        free(callback);

        return luaL_exception_extended(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    callback->next = mqtt->callbacks;
    mqtt->callbacks = callback;

    return 0;
}

static int msgArrived(void *context, char * topicName, int topicLen, MQTTAsync_message* m) {
    mqtt_userdata *mqtt = (mqtt_userdata *) context;
    if (mqtt) {

        mqtt_subs_callback *callback;

        mtx_lock(&mqtt->mtx);

        callback = mqtt->callbacks;
        while (callback) {
            if (topic_matches_sub(callback->topic, topicName)) {
                // Push argument for the callback's function
                lua_pushinteger(luaS_callback_state(callback->callback), m->payloadlen);
                lua_pushlstring(luaS_callback_state(callback->callback), m->payload, m->payloadlen);

                // see: https://www.ibm.com/support/knowledgecenter/SSFKSJ_7.5.0/com.ibm.mq.javadoc.doc/WMQMQxrCClasses/_m_q_t_t_client_8h.html?view=kc#aa42130dd069e7e949bcab37b6dce64a5
                if (topicLen == 0) {
                    lua_pushinteger(luaS_callback_state(callback->callback), strlen(topicName));
                    lua_pushstring(luaS_callback_state(callback->callback), topicName);
                } else {
                    lua_pushinteger(luaS_callback_state(callback->callback), topicLen);
                    lua_pushlstring(luaS_callback_state(callback->callback), topicName, topicLen);
                }

                luaS_callback_call(callback->callback, 4);
            }
            callback = callback->next;
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
    mqtt->client = NULL;
    mqtt->callbacks = NULL;
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

    if (!client_inited) {
        MQTTAsync_init();
        client_inited = 1;
    }

    //url is being strdup'd in MQTTClient_create
    rc = MQTTAsync_create(&mqtt->client, url, clientId, persistence, (char*)persistence_folder);
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

#ifdef OPENSSL
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
    ssl_opts.trustStore = mqtt->ca_file; //has been strdup'd already
    ssl_opts.enableServerCertAuth = (ssl_opts.trustStore != NULL);
    bcopy(&ssl_opts, &mqtt->ssl_opts, sizeof(MQTTAsync_SSLOptions));
#endif

    // Set connection context
    mqtt_conn_ctx_t ctx;

    ctx.mqtt = mqtt;
    ctx.task = xTaskGetCurrentTaskHandle();

    // Prepare connection
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.connectTimeout = MQTT_CONNECT_TIMEOUT;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    conn_opts.username = strdup(user); //needs to be strdup'd here for connectionLost usage
    conn_opts.password = strdup(password); // //needs to be strdup'd here for connectionLost usage
    conn_opts.onSuccess = connSuccess;
    conn_opts.onFailure = connFailure;
    conn_opts.automaticReconnect = 1;
    conn_opts.maxRetryInterval = 10;

    conn_opts.context = &ctx;
#ifdef OPENSSL
    conn_opts.ssl = &mqtt->ssl_opts;
#endif
    bcopy(&conn_opts, &mqtt->conn_opts, sizeof(MQTTAsync_connectOptions));

    // Try to connect
    MQTTAsync_connect(mqtt->client, &mqtt->conn_opts);

    // Wait for connection
    uint32_t rc_val;
    if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &rc_val, MQTT_CONNECT_TIMEOUT / portTICK_PERIOD_MS) == pdTRUE) {
        ctx.task = NULL;

        switch (rc_val) {
            case 1: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, unacceptable protocol version");
            case 2: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, identifier rejected");
            case 3: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, server unavailable");
            case 4: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, bad username or password");
            case 5: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "connection refused, not authorized");
            case 0xffffffff: return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "network not started");
        }
    } else {
        ctx.task = NULL;

        return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "timeout");
    }

    ctx.task = NULL;

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

    // Allocate space for subscription context
    mqtt_subs_ctx_t *ctx = malloc(sizeof(mqtt_subs_ctx_t));
    if (!ctx) {
        return luaL_exception(L, LUA_MQTT_ERR_NOT_ENOUGH_MEMORY);
    }

    ctx->mqtt = mqtt;
    ctx->topic = strdup(topic);
    ctx->qos = qos;

    // Add callback
    add_subs_callback(L, 4, mqtt, topic, qos);

    if ((rc = MQTTAsync_subscribe(mqtt->client, topic, qos, NULL)) != MQTTASYNC_SUCCESS) {
        return mqtt_emit_exeption(L, LUA_MQTT_ERR_CANT_SUBSCRIBE, rc);
    }

    return 0;
}

static int lmqtt_publish(lua_State* L) {
    int rc;
    int qos;
    size_t payload_len;
    const char *topic;
    char *payload;

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

    // Prepare message
    MQTTAsync_message msg = MQTTAsync_message_initializer;

    msg.payload = payload;
    msg.payloadlen = strlen(payload);
    msg.qos = qos;
    msg.retained = 0;

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

    if (MQTTAsync_isConnected(mqtt->client) != 1) return 0;

    // Set connection context
    mqtt_conn_ctx_t ctx;

    ctx.mqtt = mqtt;
    ctx.task = xTaskGetCurrentTaskHandle();

    // Prepare disconnection
    MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
    opts.onSuccess = discSuccess;
    opts.onFailure = discFailure;
    opts.context = &ctx;

    // Try to disconnect
    mtx_lock(&mqtt->mtx);

    MQTTAsync_disconnect(mqtt->client, &opts);

    // Wait for disconnection
    uint32_t rc_val;
    if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &rc_val, MQTT_CONNECT_TIMEOUT / portTICK_PERIOD_MS) == pdTRUE) {
        if (rc_val == 0xffffffff) {
            mtx_unlock(&mqtt->mtx);
            return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_DISCONNECT, "network not started");
        }
    } else {
        mtx_unlock(&mqtt->mtx);
        return luaL_exception_extended(L, LUA_MQTT_ERR_CANT_CONNECT, "timeout");
    }

    mtx_unlock(&mqtt->mtx);
    return 0;
}

// Destructor
static int lmqtt_client_gc(lua_State *L) {
    mqtt_subs_callback *callback;
    mqtt_subs_callback *nextcallback;

    // Get user data
    mqtt_userdata *mqtt = (mqtt_userdata *) luaL_testudata(L, 1, "mqtt.cli");
    if (mqtt) {
        // Destroy callbacks
        mtx_lock(&mqtt->mtx);

        callback = mqtt->callbacks;
        while (callback) {
            luaS_callback_destroy(callback->callback);

            nextcallback = callback->next;

            free(callback);
            callback = nextcallback;
        }
        mqtt->callbacks = NULL;

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
