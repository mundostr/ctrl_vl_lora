#pragma once

#include "config.h"

namespace bt_helpers {
	static String payload = "";

	void bt_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
		if (event == ESP_SPP_SRV_OPEN_EVT) {
			config::client_connected = true;
			config::client_just_connected = true;
		}

		if (event == ESP_SPP_CLOSE_EVT) {
			config::client_connected = false;
		}
	}

	void bt_send_params() {
		int BUFFER_LENGTH = 512;
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, BUFFER_LENGTH);

		sprintf(buffer,
			"{ \"action\": \"report_params\", \"params\": { \"ret_inicio\": %d, \"tpo_trepada\": %d, \"tpo_transicion\": %d, \"tpo_vuelo\": %d, \"offset_estab\": %d, \"ang_remolque\": %d, \"ang_circular\": %d, \"ang_despegue\": %d, \"ang_transicion\": %d, \"ang_vuelo\": %d, \"ang_dt\": %d, \"estab_invert\": %d } }",
			config::parameters.ret_inicio,
			config::parameters.tpo_trepada,
			config::parameters.tpo_transicion,
			config::parameters.tpo_vuelo,
			config::parameters.offset_estab,
			config::parameters.ang_remolque,
			config::parameters.ang_circular,
			config::parameters.ang_despegue,
			config::parameters.ang_transicion,
			config::parameters.ang_vuelo,
			config::parameters.ang_dt,
			config::parameters.estab_invert
		);
		bt.println(buffer);

		config::notify_params_saved = false;
		config::client_just_connected = false;
		config::send_current_params = false;
	}

	void bt_out_ctrl() {
	}

	void bt_in_ctrl() {
		if (bt.available()) {
			char incomingChar = bt.read();
			if (incomingChar != '\n') { payload += String(incomingChar); } else { config::payload_just_received = true; }
		}

		if (config::payload_just_received) {
			int params[QT_PARAMS];
			int counter = 0;
			int lastIndex = 0;

			ESP_LOGI(TAG, "Payload: %s", payload);
			
			for (int i = 0; i < payload.length(); i++) {
				if (payload.substring(i, i + 1) == "|") {
					params[counter] = payload.substring(lastIndex, i).toInt();
					lastIndex = i + 1;
					counter++;
				}
				
				if (i == payload.length() - 1) { params[counter] = payload.substring(lastIndex, i).toInt(); }
			}

			config::init_nvs();
			config::set_parameters(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9], params[10], params[11]);
			config::init_nvs();
			config::get_parameters();
			config::blink_rate = BLINK_RATE_SAVED;
			config::reset_blink = true;
			config::blink_timer = millis();

			payload = "";
			config::payload_just_received = false;
		}
	}
}
