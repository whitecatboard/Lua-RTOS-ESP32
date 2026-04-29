/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, LoRaWAN driver for Semtech Stack
 *
 */

#include "lora.h"

#include "esp_attr.h"

#include "Commissioning.h"
#include "LmHandler.h"
#include "LmHandlerMsgDisplay.h"
#include "LmhpClockSync.h"
#include "LmhpCompliance.h"
#include "LmhpRemoteMcastSetup.h"
#include "LoRaMac.h"
#include "LoRaMacMessageTypes.h"
#include "RegionCommon.h"
#include "board.h"
#include "firmwareVersion.h"
#include "radio.h"

#include "eeprom_flash.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/projdefs.h"
#include "hex_string.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "timer.h"

#include <sys/_timeval.h>
#include <sys/driver.h>
#include <sys/mutex.h>
#include <sys/resource.h>
#include <sys/status.h>
#include <sys/syslog.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Register driver and messages
void _lora_init();

DRIVER_REGISTER_BEGIN(LORA, lora, 0, _lora_init, NULL);
DRIVER_REGISTER_ERROR(LORA, lora, NotSetup, "not setup", LORA_ERR_NOT_SETUP);
DRIVER_REGISTER_ERROR(LORA, lora, NotEnoughtMemory, "not enough memory", LORA_ERR_NO_MEM);
DRIVER_REGISTER_ERROR(LORA, lora, UnexpectedResponse, "unexpected response", LORA_ERR_UNEXPECTED_RESPONSE);
DRIVER_REGISTER_ERROR(LORA, lora, InvalidArgument, "invalid argument", LORA_ERR_INVALID_ARGUMENT);
DRIVER_REGISTER_ERROR(LORA, lora, JoinFail, "join failed", LORA_ERR_JOIN_ERROR);
DRIVER_REGISTER_ERROR(LORA, lora, NotJoined, "not joined", LORA_ERR_NOT_JOINED);
DRIVER_REGISTER_ERROR(LORA, lora, MACError, "MAC error", LORA_ERR_MAC_ERROR);
DRIVER_REGISTER_ERROR(LORA, lora, TransmissionFailNACK, "ack not received", LORA_ERR_TX_ERROR_NACK);
DRIVER_REGISTER_ERROR(LORA, lora, TransmissionFail, "transmission error", LORA_ERR_TX_ERROR);
DRIVER_REGISTER_ERROR(LORA, lora, Timeout, "timeout", LORA_ERR_TIMEOUT);
DRIVER_REGISTER_END(LORA, lora, 0, _lora_init, NULL);

// Enable (1) / disable (0) Semtech stack callback's display messages
#define LORA_DISPLAY_MSG 0

/**
 * The current offset in seconds between GPS time and Coordinated Universal Time (UTC).
 *
 * GPS time is a continuous time scale that does not account for the leap seconds periodically added to UTC to
 * compensate for the Earth's rotation. As of the current epoch, GPS time is 18 seconds ahead of UTC. This constant
 * is used in lora_device_time_req to convert the GPS-based timestamp received from the network into a standard
 * Unix UTC timestamp for the system clock.
 */
#define LORA_GPS_LEAP_SECONDS 18

/**
 * Defines the stack size for the internal LoRa worker task.
 *
 * The stack size is conditionally adjusted based on whether message logging is enabled. If LORA_DISPLAY_MSG is
 * defined, an additional 20 KB is added to the base configuration to accommodate the stack-heavy requirements
 * of formatted string printing (e.g., printf) and logging utilities. Otherwise, the base size is used.
 */
#define LORA_TASK_BASE_STACK_SIZE (4 * 1024)

#if LORA_DISPLAY_MSG
#define LORA_TASK_STACK_SIZE LORA_TASK_BASE_STACK_SIZE + 20 * 1024
#else
#define LORA_TASK_STACK_SIZE LORA_TASK_BASE_STACK_SIZE
#endif

typedef enum {
  LORAClassA,
  LORAClassB,
  LORAClassC,
} lora_class_t;

typedef enum {
  LORADriverOpNone,
  LORADriverOpTx,
  LORADriverOpJoin,
  LORADriverOpDeviceTimeReq,
  LORADriverOpLinkCheckReq,
  LORADriverOpDeviceClassReq,
  LORADriverOpDeviceModeInd,
} lora_driver_op_t;

typedef enum {
  LORADriverOpResultUnknown,
  LORADriverOpResultSuccess,
  LORADriverOpResultError,
} lora_driver_op_result_t;

/**
 * @brief Global driver context for managing the LoRaWAN stack state, synchronization, and asynchronous operations.
 */
typedef struct {
  /** @brief Initialization flag. Set to 1 when the driver and the LoRaWAN stack have been successfully configured. */
  uint8_t setup;

  /** @brief Mutex used to synchronize access to the non-thread-safe LoRaMAC stack from different caller tasks. */
  struct mtx lora_mtx;

  /** @brief Handle for the internal LoRa worker task that processes stack events and timer interrupts. */
  TaskHandle_t lora_task_h;

  /** @brief Microsecond timestamp (ESP timer) marking when the next regular (throttled) uplink is permitted. */
  int64_t next_allowed_regular_tx_us;

  /** @brief The currently active asynchronous operation identifier (e.g., Join, Transmit, or MAC request). */
  lora_driver_op_t op;

  /** @brief The outcome of the current operation, typically populated by stack callbacks like OnTxDone or OnRxDone. */
  lora_driver_op_result_t op_result;

  /** @brief Handle of the FreeRTOS task that initiated the operation, used for unblocking via task notifications. */
  TaskHandle_t op_task;

  /** @brief Array for operation-specific metadata such as RSSI, SNR, frame counters, airtime, or MAC error codes. */
  uint32_t op_data[4];

  /** @brief Handle for the ESP-IDF Queue used to store incoming downlink packets. */
  QueueHandle_t downq_h;
} lora_ctx_t;

/**
 * @brief Global driver context.
 */
static lora_ctx_t lora_ctx = {0};

/*
 * Semtech stack callbacks declaration
 */

extern const char *MacStatusStrings[];

static void OnMacProcessNotify(void);
static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size);
static void OnNetworkParametersChange(CommissioningParams_t *params);
static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn);
static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn);
static void OnJoinRequest(LmHandlerJoinParams_t *params);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnClassChange(DeviceClass_t deviceClass);
static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t *params);
static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection);
static void OnLinkCheckRequest(LmHandlerLinkCheckParams_t *params);

static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

static const LmHandlerCallbacks_t LmHandlerCallbacks = {
    .GetBatteryLevel = NULL,
    .GetTemperature = NULL,
    .GetRandomSeed = BoardGetRandomSeed,
    .OnMacProcess = OnMacProcessNotify,
    .OnNvmDataChange = OnNvmDataChange,
    .OnNetworkParametersChange = OnNetworkParametersChange,
    .OnMacMcpsRequest = OnMacMcpsRequest,
    .OnMacMlmeRequest = OnMacMlmeRequest,
    .OnJoinRequest = OnJoinRequest,
    .OnTxData = OnTxData,
    .OnRxData = OnRxData,
    .OnClassChange = OnClassChange,
    .OnBeaconStatusChange = OnBeaconStatusChange,
    .OnSysTimeUpdate = OnSysTimeUpdate,
    .OnLinkCheckRequest = OnLinkCheckRequest,
};

static const LmHandlerParams_t LmHandlerParams = {
    .Region = ACTIVE_REGION,
    .AdrEnable = LORAWAN_ADR_STATE,
    .IsTxConfirmed = LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
    .TxDatarate = LORAWAN_DEFAULT_DATARATE,
    .PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .DataBufferMaxSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE,
    .DataBuffer = AppDataBuffer,
    .PingSlotPeriodicity = REGION_COMMON_DEFAULT_PING_SLOT_PERIODICITY,
};

/*
 * Helper functions
 */

/**
 * @brief Configures the current driver operation state and captures the calling task context.
 *
 * This internal helper synchronizes the driver's state machine before an asynchronous LoRaWAN operation begins. It
 * records the operation type, stores the handle of the task that initiated the request (to allow for later
 * notification/unblocking), and resets the operation result to an unknown state.
 *
 * @param op The specific LoRa driver operation to be performed (e.g., Tx, Join, or Class Request).
 */
static void _set_op(lora_driver_op_t op) {
  lora_ctx.op = op;
  lora_ctx.op_task = xTaskGetCurrentTaskHandle();
  lora_ctx.op_result = LORADriverOpResultUnknown;
}

/**
 * @brief Resets the driver's operation context to an idle state.
 *
 * This function clears the current operation state machine by setting the operation type to LORADriverOpNone,
 * nullifying the stored task handle, and resetting the operation result. It is typically called after an
 * asynchronous operation has completed (either via success, failure, or timeout) to ensure the driver is
 * ready for the next request and to prevent stale task notifications.
 */
static void _reset_op(void) {
  lora_ctx.op = LORADriverOpNone;
  lora_ctx.op_task = NULL;
  lora_ctx.op_result = LORADriverOpResultUnknown;
}

/**
 * @brief Calculates the maximum expected delay for a downlink response.
 *
 * This function queries the LoRaMAC stack for the current RX2 window delay configuration. It adds a safety margin
 * to this delay to account for the physical Time-on-Air of a potential downlink packet (especially at high
 * Spreading Factors like SF12) and internal processing time within the stack. This value is primarily used as
 * the timeout for xTaskNotifyWait when the driver is waiting for a network acknowledgment or response to a MAC command.
 *
 * @note The default RX2 delay is typically 2000ms if not otherwise configured by the network server.
 *
 * @return TickType_t The maximum expected delay converted into FreeRTOS ticks.
 */
static TickType_t _get_max_rx_delay(void) {
  MibRequestConfirm_t mibReq;
  uint32_t rx2Delay = 2000; // Default RX2 is usually 2000ms

  mibReq.Type = MIB_RECEIVE_DELAY_2;
  if (LoRaMacMibGetRequestConfirm(&mibReq) == LORAMAC_STATUS_OK) {
    rx2Delay = mibReq.Param.ReceiveDelay2;
  }

  // Margin: RX2 Delay + 2000ms (to account for SF12 Time-on-Air and processing)
  return pdMS_TO_TICKS(rx2Delay + 2000);
}

/**
 * @brief Triggers an empty uplink on Port 0 to transport pending MAC commands.
 *
 * This function initiates a LoRaWAN transmission with an empty payload on FPort 0. According to the LoRaWAN
 * specification, MAC commands (such as those generated by class changes or time requests) are transported in the
 * FOpts field of any frame or as a payload on Port 0. This function is used to "flush" the MAC command queue to
 * the Network Server without sending application data.
 *
 * @note This function is typically called after a request that modifies the stack state (e.g., LmHandlerRequestClass)
 * to ensure the Network Server is notified of the change immediately.
 *
 * @return driver_error_t* Returns NULL on success, or a pointer to a driver_error_t structure if the transmission
 * fails (e.g., due to duty cycle restrictions or stack being busy).
 */
static driver_error_t *_trigger_mac_uplink(void) {
  driver_error_t *err = NULL;

  LmHandlerAppData_t appData = {
      .Buffer = NULL,
      .BufferSize = 0,
      .Port = 0,
  };

  if (LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG) != LORAMAC_HANDLER_SUCCESS) {
    // LmHandlerSend triggers OnMacMcpsRequest synchronously before returning. If an error occurred (e.g., duty
    // cycle restricted), lora_ctx.op_result and op_data will already be populated with the specific MAC error.
    if (lora_ctx.op_result == LORADriverOpResultError) {
      err = driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR, MacStatusStrings[lora_ctx.op_data[0]]);
    } else {
      err = driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
    }
  }

  return err;
}

/**
 * @brief Main worker task for the LoRaWAN driver.
 *
 * This task manages the lifecycle of the LoRaMAC stack. It continuously calls LmHandlerProcess() to handle protocol
 * events and timers. If an asynchronous operation (like a Join or Tx) completes, it notifies the initiating task
 * with the result. When idle, the task blocks on xTaskNotifyWait to conserve CPU resources.
 *
 * Additionally, this task supports a deferred execution pattern: if the notification value is non-zero, it is
 * treated as a function pointer (handler) and executed within this task's context. This is typically used to
 * offload processing from Interrupt Service Routines (ISRs) or timer callbacks to the driver task.
 *
 * @param arg Unused FreeRTOS task parameter.
 */
static void _lora_task(void *arg) {
  uint32_t value;
  void (*handler)(void *);

  for (;;) {
    // Processes the LoRaMac and Radio events, and the NVM storage
    LmHandlerProcess();

    // If a driver operation has been completed, notify the initiating task with the result
    if ((lora_ctx.op_task != NULL) && (lora_ctx.op_result != LORADriverOpResultUnknown)) {
      xTaskNotify(lora_ctx.op_task, (uint32_t)lora_ctx.op_result, eSetValueWithOverwrite);
    }

    // Block task until required by the stack
    xTaskNotifyWait(0, 0xffffffff, &value, portMAX_DELAY);

    // Execute deferred function (if any)
    handler = (void *)value;
    if (handler) {
      // Call function handler
      handler(NULL);
    }
  }
}

void IRAM_ATTR _unblock_lora_taskFromISR(void *arg) {
  xTaskNotifyFromISR(lora_ctx.lora_task_h, (uint32_t)arg, eSetValueWithOverwrite, NULL);
}

void _unblock_lora_task(void *arg) { xTaskNotify(lora_ctx.lora_task_h, (uint32_t)arg, eSetValueWithOverwrite); }

void _lora_init() {
  // Create LoRa mutex
  mtx_init(&lora_ctx.lora_mtx, NULL, NULL, 0);

  // LoRa needs to maintain some information in RTC
  status_set(STATUS_NEED_RTC_SLOW_MEM, 0);
}

/*
 * Semtech stack callbacks implementation
 */

static void OnMacProcessNotify(void) { _unblock_lora_task(NULL); }

static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size) {
#if LORA_DISPLAY_MSG
  printf("\n###### ========== OnNvmDataChange ========== ######\n");
  DisplayNvmDataChange(state, size);
#endif
}

static void OnNetworkParametersChange(CommissioningParams_t *params) {
#if LORA_DISPLAY_MSG
  printf("\n###### ==== OnNetworkParametersChange ====== ######\n");
  DisplayNetworkParametersUpdate(params);
#endif
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn) {
#if LORA_DISPLAY_MSG
  printf("\n###### ======== OnMacMcpsRequest =========== ######\n");
  DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxIn);
#endif

  if (status == LORAMAC_STATUS_DUTYCYCLE_RESTRICTED) {
    lora_ctx.op_result = LORADriverOpResultError;
    lora_ctx.op_data[0] = status;
    lora_ctx.op_data[1] = nextTxIn;
  } else if (status != LORAMAC_STATUS_OK) {
    lora_ctx.op_result = LORADriverOpResultError;
    lora_ctx.op_data[0] = status;
  }
}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn) {
#if LORA_DISPLAY_MSG
  printf("\n###### ========= OnMacMlmeRequest ========== ######\n");
  DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxIn);
#endif
}

static void OnJoinRequest(LmHandlerJoinParams_t *params) {
#if LORA_DISPLAY_MSG
  printf("\n###### ========== OnJoinRequest ============ ######\n");
  DisplayJoinRequestUpdate(params);
#endif

  if (lora_ctx.op == LORADriverOpJoin) {
    if (params->CommissioningParams->IsOtaaActivation == true) {
      if (params->Status == LORAMAC_HANDLER_SUCCESS) {
        lora_ctx.op_result = LORADriverOpResultSuccess;
      } else {
        lora_ctx.op_result = LORADriverOpResultError;
      }
    }
  }
}

static void OnLinkCheckRequest(LmHandlerLinkCheckParams_t *params) {
  if (lora_ctx.op == LORADriverOpLinkCheckReq) {
    lora_ctx.op_result = LORADriverOpResultSuccess;
    lora_ctx.op_data[0] = (uint32_t)params->DemodMargin;
    lora_ctx.op_data[1] = (uint32_t)params->NbGateways;
  }
}

static void OnTxData(LmHandlerTxParams_t *params) {
#if LORA_DISPLAY_MSG
  printf("\n###### ============ OnTxData =============== ######\n");
  DisplayTxUpdate(params);
#endif

  if (params->IsMcpsConfirm == 0) {
    return;
  }

  if (params->AppData.BufferSize != 0) {
    if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG) {
      if (params->AckReceived) {
        lora_ctx.op_result = LORADriverOpResultSuccess;
      } else {
        lora_ctx.op_result = LORADriverOpResultError;
      }
    } else {
      lora_ctx.op_result = LORADriverOpResultSuccess;
    }
  }

  if (lora_ctx.op_result != LORADriverOpResultUnknown) {
    // Get channel frequency used in last transmission
    MibRequestConfirm_t mibGet;

    mibGet.Type = MIB_CHANNELS;
    if (LoRaMacMibGetRequestConfirm(&mibGet) == LORAMAC_STATUS_OK) {
      lora_ctx.op_data[0] = mibGet.Param.ChannelList[params->Channel].Frequency;
    } else {
      lora_ctx.op_data[0] = 0;
    }

    lora_ctx.op_data[1] = params->TxTimeOnAir;
    lora_ctx.op_data[2] = params->TxTimeOnAir * 100;
    lora_ctx.op_data[3] = params->Datarate;
  }
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params) {
#if LORA_DISPLAY_MSG
  printf("\n###### =========== OnRxData ============ ######\n");
  DisplayRxUpdate(appData, params);
#endif

  if (params->IsMcpsIndication == 1) {
    if (params->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
      if (appData->Port != 0) {
        // Prepare ring buffer item
        lora_downlink_t downlink = {
            .port = appData->Port,
            .dr = params->Datarate,
            .rssi = params->Rssi,
            .snr = params->Snr,
            .slot = params->RxSlot,
            .cnt = params->DownlinkCounter,
            .size = appData->BufferSize,
        };

        memcpy(downlink.payload, appData->Buffer, appData->BufferSize);

        // Send item to downlink queue
        xQueueSend(lora_ctx.downq_h, &downlink, 0);
      }

      if (lora_ctx.op == LORADriverOpDeviceModeInd) {
        lora_ctx.op_result = LORADriverOpResultSuccess;
      }
    }
  }
}

static void OnClassChange(DeviceClass_t deviceClass) {
  if (lora_ctx.op == LORADriverOpDeviceClassReq) {
    lora_ctx.op_result = LORADriverOpResultSuccess;
  }
}

static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t *params) {}

static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection) {
  if (lora_ctx.op == LORADriverOpDeviceTimeReq) {
    lora_ctx.op_result = LORADriverOpResultSuccess;
  }
}

/*
 * Operation functions
 */

driver_error_t *lora_setup(int band) {
#if CONFIG_LUA_RTOS_LORA_BAND_EU868
  if (band != 868) {
    return driver_error(LORA_DRIVER, LORA_ERR_INVALID_ARGUMENT, "invalid band for your location");
  }
#endif

#if CONFIG_LUA_RTOS_LORA_BAND_US915
  if (band != 915) {
    return driver_error(LORA_DRIVER, LORA_ERR_INVALID_ARGUMENT, "invalid band for your location");
  }
#endif

  mtx_lock(&lora_ctx.lora_mtx);

  if (lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return NULL;
  }

  BoardInitMcu();

  if (LmHandlerInit((LmHandlerCallbacks_t *)&LmHandlerCallbacks, (LmHandlerParams_t *)&LmHandlerParams) !=
      LORAMAC_HANDLER_SUCCESS) {
    return driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, NULL);
  }

  // Set system maximum tolerated rx error in milliseconds
  LmHandlerSetSystemMaxRxError(20);

  LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC, NULL);
  LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP, NULL);

  // Create queue for down-link data
  lora_ctx.downq_h = xQueueCreate(CONFIG_LUA_RTOS_LORA_RX_QUEUE_SIZE, sizeof(lora_downlink_t));
  if (lora_ctx.downq_h == NULL) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
  }

  // Create task
  BaseType_t xReturned =
      xTaskCreatePinnedToCore(_lora_task, "lora_stack", LORA_TASK_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LORA_TASK_PRIORITY,
                              &lora_ctx.lora_task_h, CONFIG_LUA_RTOS_LORA_TASK_CPU);

  if (xReturned != pdPASS) {
    vQueueDelete(lora_ctx.downq_h);
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
  }

  lora_ctx.setup = 1;

  mtx_unlock(&lora_ctx.lora_mtx);

  syslog(LOG_DEBUG, "lora: setup, band %d", band);

  return NULL;
}

driver_error_t *lora_mac_set(const char command, const uint8_t *value) {
  MibRequestConfirm_t mibSet;
  LoRaMacStatus_t status;
  driver_error_t *err = NULL;

  mtx_lock(&lora_ctx.lora_mtx);

  // Sanity checks
  if (!lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
  }

  switch (command) {
  case LORA_MAC_SET_DEVADDR:
    mibSet.Type = MIB_DEV_ADDR;
    mibSet.Param.DevAddr = (uint32_t)value;
    LoRaMacMibSetRequestConfirm(&mibSet);
    break;

  case LORA_MAC_SET_DEVEUI:
#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
    err =
        driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, "DevEui is hardware-locked, manual assignment is not supported");
#else
    mibSet.Type = MIB_DEV_EUI;
    hex_string_to_val((char *)value, (char *)mibSet.Param.DevEui, 8, 0);
#endif
    break;

  case LORA_MAC_SET_JOINEUI:
    mibSet.Type = MIB_JOIN_EUI;
    mibSet.Param.JoinEui = (uint8_t *)value;
    LoRaMacMibSetRequestConfirm(&mibSet);
    break;

  case LORA_MAC_SET_APPKEY:
    mibSet.Type = MIB_APP_KEY;
    mibSet.Param.AppKey = (uint8_t *)value;

    if ((status = LoRaMacMibSetRequestConfirm(&mibSet)) != LORAMAC_STATUS_OK) {
      err = driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, NULL);
    }
    break;

  case LORA_MAC_SET_NWKKEY:
    mibSet.Type = MIB_NWK_KEY;
    mibSet.Param.NwkKey = (uint8_t *)value;

    if ((status = LoRaMacMibSetRequestConfirm(&mibSet)) != LORAMAC_STATUS_OK) {
      err = driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, NULL);
    }
    break;

  case LORA_MAC_SET_DR:
    mibSet.Type = MIB_CHANNELS_DEFAULT_DATARATE;
    mibSet.Param.ChannelsDefaultDatarate = value[0];

    if ((status = LoRaMacMibSetRequestConfirm(&mibSet)) == LORAMAC_STATUS_PARAMETER_INVALID) {
      err = driver_error(LORA_DRIVER, LORA_ERR_INVALID_ARGUMENT, "invalid data rate for your region");
    } else if (status != LORAMAC_STATUS_OK) {
      err = driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, NULL);
    }

    if (err == NULL) {
      mibSet.Type = MIB_CHANNELS_DATARATE;
      mibSet.Param.ChannelsDatarate = value[0];

      if ((status = LoRaMacMibSetRequestConfirm(&mibSet)) == LORAMAC_STATUS_PARAMETER_INVALID) {
        err = driver_error(LORA_DRIVER, LORA_ERR_INVALID_ARGUMENT, "invalid data rate for your region");
      } else if (status != LORAMAC_STATUS_OK) {
        err = driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, NULL);
      }
    }

    if (err == NULL) {
      // Disable ADR
      mibSet.Type = MIB_ADR;
      mibSet.Param.AdrEnable = 0;
      LoRaMacMibSetRequestConfirm(&mibSet);
    }
    break;

  case LORA_MAC_SET_ADR:
    mibSet.Type = MIB_ADR;
    mibSet.Param.AdrEnable = value[0];
    LoRaMacMibSetRequestConfirm(&mibSet);
    break;

  case LORA_MAC_SET_NB_TRANS:
    mibSet.Type = MIB_CHANNELS_NB_TRANS;
    mibSet.Param.ChannelsNbTrans = value[0];
    LoRaMacMibSetRequestConfirm(&mibSet);
    break;

  case LORA_MAC_SET_DEVICE_CLASS: {
    lora_class_t class = value[0];

    if (class == LORAClassB) {
      mtx_unlock(&lora_ctx.lora_mtx);
      return driver_error(LORA_DRIVER, LORA_ERR_INVALID_ARGUMENT, "class B not supported");
    }

    if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET) {
      mtx_unlock(&lora_ctx.lora_mtx);
      return driver_error(LORA_DRIVER, LORA_ERR_NOT_JOINED, NULL);
    }

    _set_op(LORADriverOpDeviceClassReq);

    // Request the MAC layer to change LoRaWAN class
    if (LmHandlerRequestClass(class) != LORAMAC_HANDLER_SUCCESS) {
      _reset_op();
      mtx_unlock(&lora_ctx.lora_mtx);
      return driver_error(LORA_DRIVER, LORA_ERR_MAC_ERROR, NULL);
    }

    // Switch to class A or class C is instantaneous, OnClassChange has been yet executed from LmHandlerRequestClass
    if (lora_ctx.op_result == LORADriverOpResultSuccess) {
      // A DeviceModeInd MAC command has been queued by LmHandlerRequestClass
      _set_op(LORADriverOpDeviceModeInd);

      // Trigger an uplink to transport the MAC command. We use an empty frame on Port 0 as per LoRaWAN specifications
      if ((err = _trigger_mac_uplink()) != NULL) {
        _reset_op();
        mtx_unlock(&lora_ctx.lora_mtx);
        return err;
      }

      // Wait for the response notification (blocking).
      // The timeout is dynamically calculated based on the current RX2 delay plus a safety margin for Time-on-Air
      // and stack processing.
      uint32_t status;

      if (xTaskNotifyWait(0, 0xffffffff, &status, _get_max_rx_delay()) == pdFALSE) {
        err = driver_error(LORA_DRIVER, LORA_ERR_TIMEOUT, "no network response");
      } else if (lora_ctx.op_result != LORADriverOpResultSuccess) {
        err = driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
      }
    }

    _reset_op();
    break;
  }
  }

  mtx_unlock(&lora_ctx.lora_mtx);

  if (err == NULL) {
    _unblock_lora_task(NULL);
  }

  return err;
}

driver_error_t *lora_mac_get(const char command, uint8_t *value) {
  MibRequestConfirm_t mibGet;

  mtx_lock(&lora_ctx.lora_mtx);

  // Sanity checks
  if (!lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
  }

  switch (command) {
  case LORA_MAC_GET_DEVADDR:
    mibGet.Type = MIB_DEV_ADDR;
    LoRaMacMibGetRequestConfirm(&mibGet);
    memcpy(value, &mibGet.Param.DevAddr, LORA_MAC_DEVADDR_SIZE / 2);
    break;

  case LORA_MAC_GET_DEVEUI:
    mibGet.Type = MIB_DEV_EUI;
    LoRaMacMibGetRequestConfirm(&mibGet);
    memcpy(value, mibGet.Param.DevEui, LORA_MAC_EUI_SIZE / 2);
    break;

  case LORA_MAC_GET_JOINEUI:
    mibGet.Type = MIB_JOIN_EUI;
    LoRaMacMibGetRequestConfirm(&mibGet);
    memcpy(value, mibGet.Param.JoinEui, LORA_MAC_EUI_SIZE / 2);
    break;

  case LORA_MAC_GET_DR:
    mibGet.Type = MIB_CHANNELS_DATARATE;
    LoRaMacMibGetRequestConfirm(&mibGet);
    value[0] = mibGet.Param.ChannelsDatarate;
    break;

  case LORA_MAC_GET_ADR:
    mibGet.Type = MIB_ADR;
    LoRaMacMibGetRequestConfirm(&mibGet);
    value[0] = mibGet.Param.AdrEnable;
    break;

  case LORA_MAC_GET_DEVICE_CLASS:
    mibGet.Type = MIB_DEVICE_CLASS;
    LoRaMacMibGetRequestConfirm(&mibGet);
    value[0] = mibGet.Param.Class;
    break;

  case LORA_MAC_GET_NB_TRANS:
    mibGet.Type = MIB_CHANNELS_NB_TRANS;
    LoRaMacMibGetRequestConfirm(&mibGet);
    value[0] = mibGet.Param.ChannelsNbTrans;
    break;
  }

  mtx_unlock(&lora_ctx.lora_mtx);

  return NULL;
}

driver_error_t *lora_join() {
  mtx_lock(&lora_ctx.lora_mtx);

  // Sanity checks
  if (!lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
  }

  // Set current operation
  _set_op(LORADriverOpJoin);

  // Start LoRa join process
  LmHandlerJoin();

  // Wait for join status notification (blocking)
  uint32_t status;

  xTaskNotifyWait(0, 0xffffffff, &status, portMAX_DELAY);

  // Reset current operation
  _reset_op();

  // Check status
  if (status == LORADriverOpResultSuccess) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return NULL;
  } else if (status == LORADriverOpResultError) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_JOIN_ERROR, NULL);
  } else {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
  }
}

driver_error_t *lora_tx(int regular, int cnf, int port, const uint8_t *buffer, uint16_t size, lora_tx_ret_t *tx_ret) {
  driver_error_t *err = NULL;
  mtx_lock(&lora_ctx.lora_mtx);

  // Sanity checks
  if (!lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
  }

  // Check if handler is busy
  if (LmHandlerIsBusy()) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR, "handler is busy");
  }

  if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_JOINED, NULL);
  }

  // Regular transmissions are subjected to application throttle
  if (regular) {
    if (esp_timer_get_time() < lora_ctx.next_allowed_regular_tx_us) {
      mtx_unlock(&lora_ctx.lora_mtx);
      return driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR, "application throttle active");
    }
  }

  _set_op(LORADriverOpTx);

  // Send data
  LmHandlerAppData_t app_data = {
      .Port = port,
      .BufferSize = size,
      .Buffer = (uint8_t *)buffer,
  };

  if (LmHandlerSend(&app_data, cnf) != LORAMAC_HANDLER_SUCCESS) {
    // LmHandlerSend triggers OnMacMcpsRequest synchronously before returning. If an error occurred (e.g., duty
    // cycle restricted), lora_ctx.op_result and op_data will already be populated with the specific MAC error.
    if (lora_ctx.op_result == LORADriverOpResultError) {
      err = driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR, MacStatusStrings[lora_ctx.op_data[0]]);
    } else {
      err = driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
    }

    _reset_op();
    mtx_unlock(&lora_ctx.lora_mtx);
    return err;
  }

  // Wait for tx status notification (blocking)
  uint32_t status;

  xTaskNotifyWait(0, 0xffffffff, &status, portMAX_DELAY);

  // Check status
  if (status == LORADriverOpResultError) {
    if (cnf) {
      err = driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR_NACK, NULL);
    }
  } else if (status != LORADriverOpResultSuccess) {
    err = driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
  }

  tx_ret->frequency = lora_ctx.op_data[0];
  tx_ret->time_on_air = lora_ctx.op_data[1];
  tx_ret->time_off_air = lora_ctx.op_data[2];
  tx_ret->dr = lora_ctx.op_data[3];

  // Update application throttle
  if (regular) {
    lora_ctx.next_allowed_regular_tx_us = esp_timer_get_time() + lora_ctx.op_data[2] * 1000;
  }

  _reset_op();
  mtx_unlock(&lora_ctx.lora_mtx);
  return err;
}

driver_error_t *lora_device_time_req(void) {
  driver_error_t *err = NULL;
  mtx_lock(&lora_ctx.lora_mtx);

  // Sanity checks
  if (!lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
  }

  if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_JOINED, NULL);
  }

  _set_op(LORADriverOpDeviceTimeReq);

  // Prepare the a DeviceTimeReq MAC command in the stack
  if (LmHandlerDeviceTimeReq() != LORAMAC_HANDLER_SUCCESS) {
    _reset_op();
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR, "failed to schedule DeviceTimeReq");
  }

  // Trigger an uplink to transport the MAC command. We use an empty frame on Port 0 as per LoRaWAN specifications.
  if ((err = _trigger_mac_uplink()) != NULL) {
    _reset_op();
    mtx_unlock(&lora_ctx.lora_mtx);
    return err;
  }

  // Wait for the response notification (blocking).
  // The timeout is dynamically calculated based on the current RX2 delay plus a safety margin for Time-on-Air
  // and stack processing.
  uint32_t status;
  BaseType_t notified = xTaskNotifyWait(0, 0xffffffff, &status, _get_max_rx_delay());

  if (notified == pdFALSE) {
    err = driver_error(LORA_DRIVER, LORA_ERR_TIMEOUT, "no network response");
  } else if (lora_ctx.op_result == LORADriverOpResultSuccess) {
    // Synchronize the system clock with the stack's time
    SysTime_t sys_time = SysTimeGet();
    struct timeval tv = {.tv_sec = sys_time.Seconds - LORA_GPS_LEAP_SECONDS, .tv_usec = sys_time.SubSeconds * 1000};
    settimeofday(&tv, NULL);
  } else {
    err = driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
  }

  _reset_op();
  mtx_unlock(&lora_ctx.lora_mtx);

  return err;
}

driver_error_t *lora_link_check_req(lora_link_check_req_ret_t *link_check_req_ret) {
  driver_error_t *err = NULL;
  mtx_lock(&lora_ctx.lora_mtx);

  // Sanity checks
  if (!lora_ctx.setup) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_SETUP, NULL);
  }

  if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET) {
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_NOT_JOINED, NULL);
  }

  _set_op(LORADriverOpLinkCheckReq);

  // Prepare the a LinkCheckReq MAC command in the stack
  MlmeReq_t mlmeReq = {.Type = MLME_LINK_CHECK};

  if (LoRaMacMlmeRequest(&mlmeReq) != LORAMAC_STATUS_OK) {
    _reset_op();
    mtx_unlock(&lora_ctx.lora_mtx);
    return driver_error(LORA_DRIVER, LORA_ERR_TX_ERROR, "failed to schedule LinkCheckReq");
  }

  // Trigger an uplink to transport the MAC command. We use an empty frame on Port 0 as per LoRaWAN specifications.
  if ((err = _trigger_mac_uplink()) != NULL) {
    _reset_op();
    mtx_unlock(&lora_ctx.lora_mtx);
    return err;
  }

  // Wait for the response notification (blocking).
  // The timeout is dynamically calculated based on the current RX2 delay plus a safety margin for Time-on-Air
  // and stack processing.
  uint32_t status;
  BaseType_t notified = xTaskNotifyWait(0, 0xffffffff, &status, _get_max_rx_delay());

  if (notified == pdFALSE) {
    err = driver_error(LORA_DRIVER, LORA_ERR_TIMEOUT, "no network response");
  } else if (lora_ctx.op_result == LORADriverOpResultSuccess) {
    link_check_req_ret->demod_margin = lora_ctx.op_data[0];
    link_check_req_ret->nb_gateways = lora_ctx.op_data[1];
  } else {
    err = driver_error(LORA_DRIVER, LORA_ERR_UNEXPECTED_RESPONSE, NULL);
  }

  _reset_op();
  mtx_unlock(&lora_ctx.lora_mtx);

  return err;
}

QueueHandle_t lora_get_rx_queue_h(void) {
  QueueHandle_t h = NULL;

  mtx_lock(&lora_ctx.lora_mtx);

  if (lora_ctx.setup) {
    h = lora_ctx.downq_h;
  }

  mtx_unlock(&lora_ctx.lora_mtx);

  return h;
}