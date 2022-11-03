#pragma once

#include "config.h"

namespace tasks_helpers {
	void blink_ctrl(void *param) {
		for(;;) {
			config::led_active = !config::led_active;
			digitalWrite(ONBOARD_LED, config::led_active);
			vTaskDelay(config::blink_rate / portTICK_PERIOD_MS);
			// vTaskDelete(handle_blink);
		}
	}

	void init_blink(void) {
		xTaskCreatePinnedToCore(blink_ctrl, "blink_ctrl", 1024 * 1, NULL, 1, &handle_blink, 1);
	}
}