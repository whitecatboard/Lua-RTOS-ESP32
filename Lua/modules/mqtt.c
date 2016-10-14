#include "whitecat.h"

#if LUA_USE_MQTT
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"

#include "mqtt/MQTTClient.h"
#include "mqtt/MQTTClientPersistence.h"

#include "FreeRTOS.h"
#include "task.h"

#include <errno.h>
#include <sys/mutex.h>

// Module function map
#define MIN_OPT_LEVEL 2

static int client_inited = 0;

typedef struct {
    char *topic;
    int callback;
    void *next;
} mqtt_subs_callback;

typedef struct {
    lua_State *L;
    struct mtx callback_mtx;
    
    MQTTClient_connectOptions conn_opts;
    MQTTClient client;
    
    mqtt_subs_callback *callbacks;
} mqtt_userdata;

static int add_subs_callback(mqtt_userdata *mqtt, const char *topic, int call) {
    mqtt_subs_callback *callback;
    
    // Create and populate callback structure
    callback = (mqtt_subs_callback *)malloc(sizeof(mqtt_subs_callback));
    if (!callback) {
        errno = ENOMEM;
        return -1;
    }
    
    callback->topic = (char *)malloc(strlen(topic));
    if (!callback->topic) {
        errno = ENOMEM;
        free(callback);
        return -1;
    }
    
    strcpy(callback->topic, topic);
    
    callback->callback = call;
    
    mtx_lock(&mqtt->callback_mtx);
    callback->next = mqtt->callbacks;
    mqtt->callbacks = callback;
    mtx_unlock(&mqtt->callback_mtx);
    
    return 0;
}

static int remove_subs_callback(mqtt_userdata *mqtt, const char *topic) {
    
}


void connlost(void *context, char *cause) {
    mqtt_userdata *mqtt = (mqtt_userdata *)context;

    while (MQTTClient_connect(mqtt->client, &mqtt->conn_opts) < 0) {
        delay(1000);
    }
}

int messageArrived(void *context, char * topicName, int topicLen, MQTTClient_message* m) {
    mqtt_userdata *mqtt = (mqtt_userdata *)context;
    mqtt_subs_callback *callback;
    int call = 0;
    
    mtx_lock(&mqtt->callback_mtx);

    callback = mqtt->callbacks;
    while (callback) {
        if (strcmp(callback->topic, topicName) == 0) {
            call = callback->callback;
            if (call != LUA_NOREF) {
                lua_rawgeti(mqtt->L, LUA_REGISTRYINDEX, call);
                lua_pushinteger(mqtt->L, m->payloadlen);
                lua_pushlstring(mqtt->L, m->payload, m->payloadlen);
                lua_call(mqtt->L, 2, 0);
            }
        }
        
        callback = callback->next;
    }

    mtx_unlock(&mqtt->callback_mtx);
    
    MQTTClient_freeMessage(&m);
    MQTTClient_free(topicName);
    
    return 1;
}

static int mqtt_connect( lua_State* L );
static int mqtt_publish( lua_State* L );
static int mqtt_subscribe( lua_State* L );

// Lua: result = setup( id, clock )
static int mqtt_client( lua_State* L ){
    int rc = 0;
    const char *host;
    const char *clientId;
    int port;
    int id;
    int secure;
    size_t lenClientId, lenHost;
    mqtt_userdata *mqtt;
    char url[100];
        
    clientId = luaL_checklstring( L, 1, &lenClientId );
    host = luaL_checklstring( L, 2, &lenHost );
    port = luaL_checkinteger( L, 3 );

    luaL_checktype(L, 4, LUA_TBOOLEAN);
    secure = lua_toboolean( L, 4 );
    
    // Allocate mqtt structure and initialize
    mqtt = (mqtt_userdata *)lua_newuserdata(L, sizeof(mqtt_userdata));
    mqtt->L = L;
    mqtt->callbacks = NULL;
    mtx_init(&mqtt->callback_mtx, NULL, NULL, 0);
    
    // Calculate uri
    sprintf(url, "%s:%d", host, port);
    
    if (!client_inited) {
        MQTTClient_init();
        client_inited = 1;
    }
    
    rc = MQTTClient_create(&mqtt->client, url, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc < 0){
        return luaL_error( L, "can't create client" );
    }

    rc = MQTTClient_setCallbacks(mqtt->client, mqtt, NULL, messageArrived, NULL);
    if (rc < 0){
        return luaL_error( L, "can't setting callbacks" );
    }

   luaL_getmetatable(L, "mqtt");
   lua_setmetatable(L, -2);

    return 1;
}

static int mqtt_connect( lua_State* L ) {
    int rc;
    const char *user;
    const char *password;
    mqtt_userdata *mqtt = NULL;
    
    mqtt = (mqtt_userdata *)luaL_checkudata(L, 1, "mqtt");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");
    
    user = luaL_checkstring( L, 2 );
    password = luaL_checkstring( L, 3  );

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    
    conn_opts.connectTimeout = 4;
    conn_opts.keepAliveInterval = 60;
    conn_opts.reliable = 0;
    conn_opts.cleansession = 0;
    conn_opts.username = "";
    conn_opts.password = "";
    ssl_opts.enableServerCertAuth = 0;
    conn_opts.ssl = &ssl_opts;

    bcopy(&conn_opts, &mqtt->conn_opts, sizeof(MQTTClient_connectOptions));
    
    rc = MQTTClient_connect(mqtt->client, &mqtt->conn_opts);
    if (rc < 0){
        return luaL_error( L, "can't connect" );
    }    
    
    return 0;
}

static int mqtt_subscribe( lua_State* L ) {
    int rc;
    int qos;
    const char *topic;
    int callback = 0;
    
    mqtt_userdata *mqtt = NULL;
    
    mqtt = (mqtt_userdata *)luaL_checkudata(L, 1, "mqtt");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");
    
    topic = luaL_checkstring( L, 2 );
    qos = luaL_checkinteger( L, 3 );
    
    luaL_checktype(L, 4, LUA_TFUNCTION);

    // Copy argument (function) to the top of stack
    lua_pushvalue(L, 4); 

    // Copy function reference
    callback = luaL_ref(L, LUA_REGISTRYINDEX);

    add_subs_callback(mqtt, topic, callback);        
    
    rc = MQTTClient_subscribe(mqtt->client, topic, qos);
    if (rc == 0) {
        return 0;
    } else {
        return luaL_error( L, "can't subscribe to topic" );
    }
}

static int mqtt_publish( lua_State* L ) {
    int rc;
    int qos;
    size_t payload_len;
    const char *topic;
    char *payload;

    mqtt_userdata *mqtt = NULL;
    
    mqtt = (mqtt_userdata *)luaL_checkudata(L, 1, "mqtt");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");
    
    topic = luaL_checkstring( L, 2 );
    payload = (char *)luaL_checklstring( L, 3, &payload_len );
    qos = luaL_checkinteger( L, 4 );
    
    rc = MQTTClient_publish(mqtt->client, topic, payload_len, payload, 
            qos, 0, NULL);

    if (rc == 0) {
        return 0;
    } else {
        return luaL_error( L, "can't publish to topic" );
    }
}

static int mqtt_disconnect( lua_State* L ) {
    int rc;

    mqtt_userdata *mqtt = NULL;
    
    mqtt = (mqtt_userdata *)luaL_checkudata(L, 1, "mqtt");
    luaL_argcheck(L, mqtt, 1, "mqtt expected");
    
    rc = MQTTClient_disconnect(mqtt->client, 0);
    if (rc == 0) {
        return 0;
    } else {
        return luaL_error( L, "can't disconnect" );
    }
}

static int f_gc (lua_State *L) {
    mqtt_userdata *mqtt = NULL;
    mqtt_subs_callback *callback;
    mqtt_subs_callback *nextcallback;
    int call = 0;
    
    mqtt = (mqtt_userdata *)luaL_testudata(L, 1, "mqtt");    
    if (mqtt) {        
        // Destroy callbacks
        mtx_lock(&mqtt->callback_mtx);

        callback = mqtt->callbacks;
        while (callback) {
            luaL_unref(L, LUA_REGISTRYINDEX, callback->callback);
            nextcallback = callback->next;
        
            free(callback);
            callback = nextcallback;
        }

        mtx_unlock(&mqtt->callback_mtx);

        // Disconnect and destroy client
        MQTTClient_disconnect(mqtt->client, 0);
        MQTTClient_destroy(&mqtt->client);        
        
        mtx_destroy(&mqtt->callback_mtx);
    }
   
    return 0;
}

const luaL_Reg mqtt_client_map[] = 
{
  {"__gc", f_gc},
  { "client",  mqtt_client },
  { "connect",  mqtt_connect },
  { "disconnect",  mqtt_disconnect },
  { "subscribe",  mqtt_subscribe },
  { "publish",  mqtt_publish },
  { NULL, NULL}
};

const luaL_Reg mqtt_map[] = {
  { NULL, NULL }
};

LUALIB_API int luaopen_mqtt( lua_State *L ) {
    int n;
    luaL_register( L, AUXLIB_MQTT, mqtt_map );
    
    // Set it as its own metatable
    lua_pushvalue( L, -1 );
    lua_setmetatable( L, -2 );

    // create metatable
    luaL_newmetatable(L, "mqtt");

    // Module constants  
    MOD_REG_INTEGER( L, "QOS0", 0 );
    MOD_REG_INTEGER( L, "QOS1", 1 );
    MOD_REG_INTEGER( L, "QOS2", 2 );

    
    // metatable.__index = metatable
    lua_pushliteral(L, "__index");
    lua_pushvalue(L,-2);
    lua_rawset(L,-3);
    
    // Setup the methods inside metatable
    luaL_register( L, NULL, mqtt_client_map );

    return 1;  
}
#endif