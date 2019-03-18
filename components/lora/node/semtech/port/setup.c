#include "spi.h"
#include "sx1276-board.h"

#include "LoRaMac.h"
#include "Commissioning.h"

#include <sys/driver.h>

#define ACTIVE_REGION LORAMAC_REGION_EU868

#define LORAWAN_DEFAULT_DATARATE                    DR_5

/*!
 * LoRaWAN application port
 */
#define LORAWAN_APP_PORT                            2

static uint8_t DevEui[] = LORAWAN_DEVICE_EUI;
static uint8_t AppEui[] = LORAWAN_APPLICATION_EUI;
static uint8_t AppKey[] = LORAWAN_APPLICATION_KEY;

static uint8_t NwkSKey[] = LORAWAN_NWKSKEY;
static uint8_t AppSKey[] = LORAWAN_APPSKEY;
static uint32_t DevAddr = LORAWAN_DEVICE_ADDRESS;

static uint8_t IsTxConfirmed = 0;

/*!
 * Application port
 */
static uint8_t AppPort = 1;

/*!
 * User application data size
 */
static uint8_t AppDataSize = 16;

static uint8_t AppData[243];

const char* MacStatusStrings[] =
{
    "OK",                            // LORAMAC_STATUS_OK
    "Busy",                          // LORAMAC_STATUS_BUSY
    "Service unknown",               // LORAMAC_STATUS_SERVICE_UNKNOWN
    "Parameter invalid",             // LORAMAC_STATUS_PARAMETER_INVALID
    "Frequency invalid",             // LORAMAC_STATUS_FREQUENCY_INVALID
    "Datarate invalid",              // LORAMAC_STATUS_DATARATE_INVALID
    "Frequency or datarate invalid", // LORAMAC_STATUS_FREQ_AND_DR_INVALID
    "No network joined",             // LORAMAC_STATUS_NO_NETWORK_JOINED
    "Length error",                  // LORAMAC_STATUS_LENGTH_ERROR
    "Region not supported",          // LORAMAC_STATUS_REGION_NOT_SUPPORTED
    "Skipped APP data",              // LORAMAC_STATUS_SKIPPED_APP_DATA
    "Duty-cycle restricted",         // LORAMAC_STATUS_DUTYCYCLE_RESTRICTED
    "No channel found",              // LORAMAC_STATUS_NO_CHANNEL_FOUND
    "No free channel found",         // LORAMAC_STATUS_NO_FREE_CHANNEL_FOUND
    "Busy beacon reserved time",     // LORAMAC_STATUS_BUSY_BEACON_RESERVED_TIME
    "Busy ping-slot window time",    // LORAMAC_STATUS_BUSY_PING_SLOT_WINDOW_TIME
    "Busy uplink collision",         // LORAMAC_STATUS_BUSY_UPLINK_COLLISION
    "Crypto error",                  // LORAMAC_STATUS_CRYPTO_ERROR
    "FCnt handler error",            // LORAMAC_STATUS_FCNT_HANDLER_ERROR
    "MAC command error",             // LORAMAC_STATUS_MAC_COMMAD_ERROR
    "ClassB error",                  // LORAMAC_STATUS_CLASS_B_ERROR
    "Confirm queue error",           // LORAMAC_STATUS_CONFIRM_QUEUE_ERROR
    "Multicast group undefined",     // LORAMAC_STATUS_MC_GROUP_UNDEFINED
    "Unknown error",                 // LORAMAC_STATUS_ERROR
};

static void McpsConfirm( McpsConfirm_t *mcpsConfirm ) {
	printf("McpsConfirm\r\n");

}

static void McpsIndication( McpsIndication_t *mcpsIndication ) {
	printf("McpsIndication\r\n");

}

static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm ) {
	printf("MlmeConfirm\r\n");

}

static void MlmeIndication( MlmeIndication_t *mlmeIndication ) {
	printf("MlmeIndication\r\n");

}

static uint8_t BoardGetBatteryLevel() {
	return 0xff;
}

static LoRaMacStatus_t check(LoRaMacStatus_t status) {
    if (status != LORAMAC_STATUS_OK) {
        printf("check: %s\r\n", MacStatusStrings[status]);
    }

	return status;
}

static void PrepareTxFrame( uint8_t port )
{
    AppData[0] = 0;
    AppData[1] = 1;
    AppData[2] = 2;
    AppData[3] = 3;
    AppData[4] = 4;
    AppData[5] = 5;

    AppDataSize = 6;
}

static bool SendFrame( void )
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;

    if( LoRaMacQueryTxPossible( AppDataSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
    }
    else
    {
    	printf("possible\r\n");

        if( IsTxConfirmed == false )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = AppPort;
            mcpsReq.Req.Unconfirmed.fBuffer = AppData;
            mcpsReq.Req.Unconfirmed.fBufferSize = AppDataSize;
            mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = AppPort;
            mcpsReq.Req.Confirmed.fBuffer = AppData;
            mcpsReq.Req.Confirmed.fBufferSize = AppDataSize;
            mcpsReq.Req.Confirmed.NbTrials = 8;
            mcpsReq.Req.Confirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
        }
    }

    if( check(LoRaMacMcpsRequest( &mcpsReq )) == LORAMAC_STATUS_OK )
    {
        return false;
    }
    return true;
}

void semtech_setup() {
    LoRaMacPrimitives_t LoRaMacPrimitives;
    LoRaMacCallback_t LoRaMacCallbacks;
    MibRequestConfirm_t mibReq;

    RtcInit();

#if (CONFIG_LUA_RTOS_LORA_SPI == 2)
    SpiInit(&SX1276.Spi, CONFIG_LUA_RTOS_LORA_SPI, CONFIG_LUA_RTOS_SPI2_MOSI, CONFIG_LUA_RTOS_SPI2_MISO, CONFIG_LUA_RTOS_SPI2_CLK, CONFIG_LUA_RTOS_LORA_CS);
#endif

#if (CONFIG_LUA_RTOS_LORA_SPI == 3)
    SpiInit(&SX1276.Spi, CONFIG_LUA_RTOS_LORA_SPI, CONFIG_LUA_RTOS_SPI3_MOSI, CONFIG_LUA_RTOS_SPI3_MISO, CONFIG_LUA_RTOS_SPI3_CLK, CONFIG_LUA_RTOS_LORA_CS);
#endif

    SX1276IoInit();

    LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
    LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
    LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
    LoRaMacPrimitives.MacMlmeIndication = MlmeIndication;
    LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;

    LoRaMacStatus_t status;

    check(LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, ACTIVE_REGION));

    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = false;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    mibReq.Type = MIB_PUBLIC_NETWORK;
    mibReq.Param.EnablePublicNetwork = true;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    mibReq.Type = MIB_NET_ID;
    mibReq.Param.NetID = LORAWAN_NETWORK_ID;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    mibReq.Type = MIB_DEV_ADDR;
    mibReq.Param.DevAddr = DevAddr;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    mibReq.Type = MIB_F_NWK_S_INT_KEY;
    mibReq.Param.FNwkSIntKey = NwkSKey;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    mibReq.Type = MIB_APP_S_KEY;
    mibReq.Param.AppSKey = AppSKey;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    mibReq.Type = MIB_NETWORK_ACTIVATION;
    mibReq.Param.NetworkActivation = ACTIVATION_TYPE_ABP;
    check(LoRaMacMibSetRequestConfirm( &mibReq ));

    PrepareTxFrame( AppPort );

    SendFrame( );
}
