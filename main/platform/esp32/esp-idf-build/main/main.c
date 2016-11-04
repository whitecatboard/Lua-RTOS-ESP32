#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

void IRAM_ATTR _newTick() {
	
}

void app_main() {
	nvs_flash_init();
	system_init();

	printf("Hello world!\r\n");
}
