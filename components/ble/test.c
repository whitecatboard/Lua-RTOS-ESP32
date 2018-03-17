#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_deep_sleep.h"
#include "bt.h"

#define SLEEP_SECS									30 /* 30 secs to sleep */
#define HCI_H4_CMD_PREAMBLE_SIZE           			(4)

/*  HCI Command opcode group field(OGF) */
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS    			(0x03 << 10)            /* 0x0C00 */
#define HCI_GRP_BLE_CMDS                   			(0x08 << 10)

#define HCI_RESET                          			(0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_BLE_WRITE_ADV_ENABLE           			(0x000A | HCI_GRP_BLE_CMDS)
#define HCI_BLE_WRITE_ADV_PARAMS           			(0x0006 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_WRITE_ADV_DATA             			(0x0008 | HCI_GRP_BLE_CMDS)

#define HCIC_PARAM_SIZE_WRITE_ADV_ENABLE        	(1)
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS    	(15)
#define HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA      	(31)

#define BD_ADDR_LEN     							(6) /* Device address length */
typedef uint8_t bd_addr_t[BD_ADDR_LEN];         		/* Device address */

#define UINT16_TO_STREAM(p, u16) 					{*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   					{*(p)++ = (uint8_t)(u8);}
#define BDADDR_TO_STREAM(p, a)   					{int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) 					{int ijk; for (ijk = 0; ijk < len; ijk++) *(p)++ = (uint8_t) a[ijk];}

enum {

	H4_TYPE_COMMAND = 1,
	H4_TYPE_ACL     = 2,
	H4_TYPE_SCO     = 3,
	H4_TYPE_EVENT   = 4

};

static uint8_t hci_cmd_buf[128];

static void controller_rcv_pkt_ready(void) {

    printf("controller rcv pkt ready\n");

}

static int host_rcv_pkt(uint8_t *data, uint16_t len) {

    printf("host rcv pkt: ");
    for (uint16_t i = 0; i < len; i++) printf("%02x", data[i]);
    printf("\n");
    return 0;

}

static esp_vhci_host_callback_t vhci_host_cb = {

    controller_rcv_pkt_ready,
    host_rcv_pkt

};

static uint16_t make_cmd_reset(uint8_t *buf) {

    UINT8_TO_STREAM 	(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM 	(buf, HCI_RESET);
    UINT8_TO_STREAM 	(buf, 0);

    return HCI_H4_CMD_PREAMBLE_SIZE;

}

static uint16_t make_cmd_ble_set_adv_enable (uint8_t *buf, uint8_t adv_enable) {

    UINT8_TO_STREAM 	(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM 	(buf, HCI_BLE_WRITE_ADV_ENABLE);
    UINT8_TO_STREAM  	(buf, HCIC_PARAM_SIZE_WRITE_ADV_ENABLE);
    UINT8_TO_STREAM 	(buf, adv_enable);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_ADV_ENABLE;


static uint16_t make_cmd_ble_set_adv_param (uint8_t *buf, uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type, uint8_t addr_type_own, uint8_t addr_type_dir, bd_addr_t direct_bda, uint8_t channel_map, uint8_t adv_filter_policy) {

    UINT8_TO_STREAM 	(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM 	(buf, HCI_BLE_WRITE_ADV_PARAMS);
    UINT8_TO_STREAM  	(buf, HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS );
    UINT16_TO_STREAM 	(buf, adv_int_min);
    UINT16_TO_STREAM 	(buf, adv_int_max);
    UINT8_TO_STREAM 	(buf, adv_type);
    UINT8_TO_STREAM 	(buf, addr_type_own);
    UINT8_TO_STREAM 	(buf, addr_type_dir);
    BDADDR_TO_STREAM 	(buf, direct_bda);
    UINT8_TO_STREAM 	(buf, channel_map);
    UINT8_TO_STREAM 	(buf, adv_filter_policy);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_BLE_WRITE_ADV_PARAMS;

}


static uint16_t make_cmd_ble_set_adv_data(uint8_t *buf, uint8_t data_len, uint8_t *p_data) {

    UINT8_TO_STREAM (buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM (buf, HCI_BLE_WRITE_ADV_DATA);
    UINT8_TO_STREAM  (buf, HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA + 1);

    memset(buf, 0, HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA);

    if (p_data != NULL && data_len > 0) {

        if (data_len > HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA) data_len = HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA;
        UINT8_TO_STREAM (buf, data_len);
        ARRAY_TO_STREAM (buf, p_data, data_len);

    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_BLE_WRITE_ADV_DATA + 1;

}

static void hci_cmd_send_reset(void) {

    uint16_t sz = make_cmd_reset (hci_cmd_buf);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);

}

static void hci_cmd_send_ble_adv_start(void) {

    uint16_t sz = make_cmd_ble_set_adv_enable (hci_cmd_buf, 1);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);

}

static void hci_cmd_send_ble_set_adv_param(void) {

    uint16_t adv_intv_min 		= 256; // 160ms
    uint16_t adv_intv_max 		= 256; // 160ms
    uint8_t adv_type 			= 0; // connectable undirected advertising (ADV_IND)
    uint8_t own_addr_type 		= 0; // Public Device Address
    uint8_t peer_addr_type 		= 0; // Public Device Address
    uint8_t peer_addr[6] 		= {0x80, 0x81, 0x82, 0x83, 0x84, 0x85}; /* peer address ?? */
    uint8_t adv_chn_map 		= 0x07; // 37, 38, 39
    uint8_t adv_filter_policy 	= 0; // Process All Conn and Scan

    uint16_t sz = make_cmd_ble_set_adv_param(hci_cmd_buf,
                  adv_intv_min,
                  adv_intv_max,
                  adv_type,
                  own_addr_type,
                  peer_addr_type,
                  peer_addr,
                  adv_chn_map,
                  adv_filter_policy);

    esp_vhci_host_send_packet(hci_cmd_buf, sz);

}

static void hci_cmd_send_ble_set_adv_data(void) {

	uint8_t adv_data[31];
    uint8_t adv_data_len = 31;
	//const uint8_t *addr = esp_bt_dev_get_address(); core dump TODO !!

	  // The Eddystone Service UUID, 0xFEAA. See https://github.com/google/eddystone
      // 0000 FEAA - 0000 - 1000 - 8000 - 00 80 5F 9B 34 FB

	//0201060303aafe1716aafe00ed3f3b51b60b88ef9949e5cf484185ae0b0000

    adv_data[0]  = 0x02; /* Length	Flags. CSS v5, Part A, ยง 1.3 */
    adv_data[1]  = 0x01; /* Flags data type value */
    adv_data[2]  = 0x06; /* Flags data GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04 */
    adv_data[3]  = 0x03; /* Length	Complete list of 16-bit Service UUIDs. Ibid. ยง 1.1 */
    adv_data[4]  = 0x03; /* Complete list of 16-bit Service UUIDs data type value */
    adv_data[5]  = 0xAA; /* 16-bit Eddystone UUID 0xAA LSB */
    adv_data[6]  = 0xFE; /* 16-bit Eddystone UUID 0xFE MSB */
    adv_data[7]  = 0x17; /* Length	Service Data. Ibid. ยง 1.11 23 bytes from this point */
    adv_data[8]  = 0x16; /* Service Data data type value */
    adv_data[9]  = 0xAA; /* 16-bit Eddystone UUID 0xAA LSB */
    adv_data[10] = 0xFE; /* 16-bit Eddystone UUID 0xFE MSB */
    adv_data[11] = 0x00; /* Frame Type Eddystone UUID Value = 0x00 */
    adv_data[12] = 0xED; /* Ranging Data	Calibrated Tx power at 0 m  -60dBm meter + 41dBm = -19dBm */

	/*3f 3b 51 b6 0b 88 ef 99 49 e5 cf 48 41 85 ae 0b*/

    adv_data[13] = 0x3F; /* 10-byte Namespace uuid generated */
    adv_data[14] = 0x3B; /* 10-byte Namespace uuid generated  */
    adv_data[15] = 0x51; /* 10-byte Namespace uuid generated  */
    adv_data[16] = 0xB6; /* 10-byte Namespace uuid generated  */
    adv_data[17] = 0x0B; /* 10-byte Namespace uuid generated  */
    adv_data[18] = 0x88; /* 10-byte Namespace uuid generated  */
    adv_data[19] = 0xEF; /* 10-byte Namespace uuid generated  */
    adv_data[20] = 0x99; /* 10-byte Namespace uuid generated  */
    adv_data[21] = 0x49; /* 10-byte Namespace uuid generated  */
    adv_data[22] = 0xE5; /* 10-byte Namespace uuid generated  */
    adv_data[23] = 0xCF; /* 6-byte Instance MAC address */
    adv_data[24] = 0x48; /* 6-byte Instance MAC address */
    adv_data[25] = 0x41; /* 6-byte Instance MAC address */
    adv_data[26] = 0x85; /* 6-byte Instance MAC address */
    adv_data[27] = 0xAE; /* 6-byte Instance MAC address */
    adv_data[28] = 0x0B; /* 6-byte Instance MAC address */
    adv_data[29] = 0x00; /* RFU Reserved for future use, must be0x00 */
    adv_data[30] = 0x00; /* RFU Reserved for future use, must be0x00 */

    printf("Eddystone adv_data [%d]=", adv_data_len);
    for (int i = 0; i < adv_data_len; i++) printf("%02x", adv_data[i]);
    printf("\n");

    uint16_t sz = make_cmd_ble_set_adv_data(hci_cmd_buf, adv_data_len, (uint8_t *)adv_data);
    esp_vhci_host_send_packet(hci_cmd_buf, sz);

}

void bleAdvtTask(void *pvParameters) {

    int frames = 0;
	int cmd_cnt = 0;
   	bool send_avail = false;
   	esp_vhci_host_register_callback(&vhci_host_cb);

//    while (frames < 20) {
	printf("BLE advt task start\n");
for(;;) {

                send_avail = esp_vhci_host_check_send_available();

        if (send_avail) {

            switch (cmd_cnt) {

            case 0: hci_cmd_send_reset();
            	cmd_cnt++;
            	break;

            case 1: hci_cmd_send_ble_set_adv_param();
            	cmd_cnt++;
            	break;

            case 2: hci_cmd_send_ble_set_adv_data();
            	cmd_cnt++;
            	break;

            case 3: hci_cmd_send_ble_adv_start();
                printf("BLE Advertise, flag_send_avail: %d, cmd_sent: %d\n", send_avail, cmd_cnt);
				cmd_cnt++;
             	break;

            }
        	vTaskDelay(1000 / portTICK_PERIOD_MS);

        }

		frames++;

    }

	printf("Eddystone Beacon sleep %i seconds...\n", SLEEP_SECS);
	esp_deep_sleep(SLEEP_SECS * 1e6);

}

void app_ble_main() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	esp_bt_controller_init(&bt_cfg);
	if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) return;
	xTaskCreatePinnedToCore(&bleAdvtTask, "bleAdvtTask", 2048, NULL, 5, NULL, 0);

}
