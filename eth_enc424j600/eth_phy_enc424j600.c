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
 * Lua RTOS, ENC424J600 ethernet driver
 *
 */

#include <string.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "enc424j600.h"
#include "sdkconfig.h"

static const char *TAG = "enc424j600";

#define PHY_CHECK(a, str, goto_tag, ...)                                          \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

typedef struct {
    esp_eth_phy_t parent;
    esp_eth_mediator_t *eth;
    uint32_t addr;
    uint32_t reset_timeout_ms;
    eth_link_t link_status;
    int reset_gpio_num;
} phy_enc424j600_t;

static esp_err_t enc424j600_set_mediator(esp_eth_phy_t *phy, esp_eth_mediator_t *eth) {
    PHY_CHECK(eth, "can't set mediator for enc424j600 to null", err);
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    enc424j600->eth = eth;
    return ESP_OK;
err:
    return ESP_ERR_INVALID_ARG;
}

static esp_err_t enc424j600_get_link(esp_eth_phy_t *phy) {
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    esp_eth_mediator_t *eth = enc424j600->eth;
	uint16_t stat1;
	uint16_t stat3;

	// Check if link status changed
    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHSTAT1, (uint32_t *)&stat1) == ESP_OK,
              "read PHSTAT1 failed", err);

	eth_link_t link = (stat1 & PHSTAT1_LLSTAT)?ETH_LINK_UP:ETH_LINK_DOWN;

	if (enc424j600->link_status != link) {
		if (link == ETH_LINK_UP) {
			// Get current speed & duplex
			eth_speed_t speed = ETH_SPEED_10M;
		    eth_duplex_t duplex = ETH_DUPLEX_HALF;
			uint8_t spddpx;

		    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHSTAT3, (uint32_t *)&stat3) == ESP_OK,
		              "read PHSTAT3 failed", err);

		    spddpx = ((stat3 & (PHSTAT3_SPDDPX2 | PHSTAT3_SPDDPX1 | PHSTAT3_SPDDPX0)) >> 2);

		    if (spddpx == 0x06) {
		    	speed = ETH_SPEED_100M;
				duplex = ETH_DUPLEX_FULL;
		    } else if (spddpx == 0x02) {
		    	speed = ETH_SPEED_100M;
				duplex = ETH_DUPLEX_HALF;
		    } else if (spddpx == 0x05) {
		    	speed = ETH_SPEED_10M;
				duplex = ETH_DUPLEX_FULL;
		    } else if (spddpx == 0x01) {
		    	speed = ETH_SPEED_10M;
				duplex = ETH_DUPLEX_HALF;
		    } else {
		    	return ESP_FAIL;
		    }

            PHY_CHECK(eth->on_state_changed(eth, ETH_STATE_SPEED, (void *)speed) == ESP_OK,
                      "change speed failed", err);
            PHY_CHECK(eth->on_state_changed(eth, ETH_STATE_DUPLEX, (void *)duplex) == ESP_OK,
                      "change duplex failed", err);
		}

        PHY_CHECK(eth->on_state_changed(eth, ETH_STATE_LINK, (void *)link) == ESP_OK,
                  "change link failed", err);

        enc424j600->link_status = link;
	}

	return ESP_OK;

	err:
	    return ESP_FAIL;
}

static esp_err_t enc424j600_reset(esp_eth_phy_t *phy) {
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    enc424j600->link_status = ETH_LINK_DOWN;
    esp_eth_mediator_t *eth = enc424j600->eth;

    // Reset phy
    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, PHCON1_PRST) == ESP_OK,
              "write PHCON1 failed", err);

    // Wait for reset complete
    uint32_t to = 0;
    uint16_t phcon1;

    for (to = 0; to < enc424j600->reset_timeout_ms / 10; to++) {
        vTaskDelay(pdMS_TO_TICKS(10));

        PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&phcon1) == ESP_OK,
                 "read PHCON1 failed", err);

        if (!(phcon1 & PHCON1_PRST)) {
        	break;
        }
    }

    PHY_CHECK(to < enc424j600->reset_timeout_ms / 10, "PHY reset timeout", err);

    return ESP_OK;

	err:
	    return ESP_FAIL;
}

static esp_err_t enc424j600_reset_hw(esp_eth_phy_t *phy)
{
#if 0
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    // set reset_gpio_num minus zero can skip hardware reset phy chip
    if (enc424j600->reset_gpio_num >= 0) {
        gpio_reset_pin(enc424j600->reset_gpio_num);
        gpio_set_direction(enc424j600->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(enc424j600->reset_gpio_num, 0);
        gpio_set_level(enc424j600->reset_gpio_num, 1);
    }
    return ESP_OK;
#else
    return ESP_OK;
#endif
}

static esp_err_t enc424j600_autonego_ctrl(esp_eth_phy_t *phy, eth_phy_autoneg_cmd_t cmd, bool *autoneg_en_stat) {
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    esp_eth_mediator_t *eth = enc424j600->eth;
	uint16_t val;

    switch (cmd) {
		case ESP_ETH_PHY_AUTONEGO_RESTART:
		    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
		              "read PHCON1 failed", err);
			val |= PHCON1_RENEG;
		    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
		              "write PHCON1 failed", err);
			break;

		case ESP_ETH_PHY_AUTONEGO_EN:
		    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
		              "read PHCON1 failed", err);
			val |= PHCON1_ANEN;
		    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
		              "write PHCON1 failed", err);
			break;

		case ESP_ETH_PHY_AUTONEGO_DIS:
		    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
		              "read PHCON1 failed", err);
			val &= ~PHCON1_ANEN;
		    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
		              "write PHCON1 failed", err);
			break;

		case ESP_ETH_PHY_AUTONEGO_G_STAT:
			*autoneg_en_stat = true;
			break;

		default:
			return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;

	err:
	    return ESP_FAIL;
}

esp_err_t enc424j600_set_speed(esp_eth_phy_t *phy, eth_speed_t speed) {
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    esp_eth_mediator_t *eth = enc424j600->eth;
    uint16_t val;

    if (speed == ETH_SPEED_10M) {
	    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
	              "read PHCON1 failed", err);
		val &= ~PHCON1_SPD100;
	    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
	              "write PHCON1 failed", err);
    } else if (speed == ETH_SPEED_100M) {
	    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
	              "read PHCON1 failed", err);
		val |= PHCON1_SPD100;
	    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
	              "write PHCON1 failed", err);
    } else {
    	return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;

	err:
	    return ESP_FAIL;
}

esp_err_t enc424j600_set_duplex(esp_eth_phy_t *phy, eth_duplex_t duplex) {
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    esp_eth_mediator_t *eth = enc424j600->eth;
    uint16_t val;

    if (duplex == ETH_DUPLEX_HALF) {
	    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
	              "read PHCON1 failed", err);
		val &= ~PHCON1_PFULDPX;
	    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
	              "write PHCON1 failed", err);
    } else if (duplex == ETH_DUPLEX_FULL) {
	    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, PHCON1, (uint32_t *)&val) == ESP_OK,
	              "read PHCON1 failed", err);
		val |= PHCON1_PFULDPX;
	    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, PHCON1, (uint32_t)val) == ESP_OK,
	              "write PHCON1 failed", err);
    } else {
    	return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;

	err:
	    return ESP_FAIL;
}

static esp_err_t enc424j600_pwrctl(esp_eth_phy_t *phy, bool enable) {
#if 0
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    esp_eth_mediator_t *eth = enc424j600->eth;
    bmcr_reg_t bmcr;
    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, ETH_PHY_BMCR_REG_ADDR, &(bmcr.val)) == ESP_OK,
              "read BMCR failed", err);
    if (!enable) {
        /* Enable IEEE Power Down Mode */
        bmcr.power_down = 1;
    } else {
        /* Disable IEEE Power Down Mode */
        bmcr.power_down = 0;
    }
    PHY_CHECK(eth->phy_reg_write(eth, enc424j600->addr, ETH_PHY_BMCR_REG_ADDR, bmcr.val) == ESP_OK,
              "write BMCR failed", err);
    PHY_CHECK(eth->phy_reg_read(eth, enc424j600->addr, ETH_PHY_BMCR_REG_ADDR, &(bmcr.val)) == ESP_OK,
              "read BMCR failed", err);
    if (!enable) {
        PHY_CHECK(bmcr.power_down == 1, "power down failed", err);
    } else {
        PHY_CHECK(bmcr.power_down == 0, "power up failed", err);
    }
    return ESP_OK;
err:
    return ESP_FAIL;
#else
    return ESP_OK;
#endif
}

static esp_err_t enc424j600_set_addr(esp_eth_phy_t *phy, uint32_t addr)
{
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    enc424j600->addr = addr;

    return ESP_OK;
}

static esp_err_t enc424j600_get_addr(esp_eth_phy_t *phy, uint32_t *addr)
{
    PHY_CHECK(addr, "addr can't be null", err);
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    *addr = enc424j600->addr;

    return ESP_OK;

err:
    return ESP_ERR_INVALID_ARG;
}

static esp_err_t enc424j600_del(esp_eth_phy_t *phy)
{
    phy_enc424j600_t *enc424j600 = __containerof(phy, phy_enc424j600_t, parent);
    free(enc424j600);

    return ESP_OK;
}

static esp_err_t enc424j600_init(esp_eth_phy_t *phy) {
    PHY_CHECK(enc424j600_reset(phy) == ESP_OK, "reset failed", err);

    return ESP_OK;

err:
    return ESP_FAIL;
}

static esp_err_t enc424j600_deinit(esp_eth_phy_t *phy)
{
#if 0
    /* Power off Ethernet PHY */
    PHY_CHECK(enc424j600_pwrctl(phy, false) == ESP_OK, "power off Ethernet PHY failed", err);
    return ESP_OK;
err:
    return ESP_FAIL;
#else
    return ESP_OK;
#endif
}

esp_eth_phy_t *esp_eth_phy_new_enc424j600(const eth_phy_config_t *config)
{
    PHY_CHECK(config, "can't set phy config to null", err);
    phy_enc424j600_t *enc424j600 = calloc(1, sizeof(phy_enc424j600_t));
    PHY_CHECK(enc424j600, "calloc enc424j600 failed", err);
    enc424j600->addr = config->phy_addr;
    enc424j600->reset_timeout_ms = config->reset_timeout_ms;
    enc424j600->reset_gpio_num = config->reset_gpio_num;
    enc424j600->link_status = ETH_LINK_DOWN;
    enc424j600->parent.reset = enc424j600_reset;
    enc424j600->parent.reset_hw = enc424j600_reset_hw;
    enc424j600->parent.init = enc424j600_init;
    enc424j600->parent.deinit = enc424j600_deinit;
    enc424j600->parent.set_mediator = enc424j600_set_mediator;
    enc424j600->parent.autonego_ctrl = enc424j600_autonego_ctrl;
    enc424j600->parent.get_link = enc424j600_get_link;
    enc424j600->parent.pwrctl = enc424j600_pwrctl;
    enc424j600->parent.get_addr = enc424j600_get_addr;
    enc424j600->parent.set_addr = enc424j600_set_addr;
    enc424j600->parent.set_speed = enc424j600_set_speed;
    enc424j600->parent.set_duplex = enc424j600_set_duplex;
    enc424j600->parent.del = enc424j600_del;
    return &(enc424j600->parent);
err:
    return NULL;
}
