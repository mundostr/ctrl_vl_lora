/*
Controlador Vuelo Libre con RDT LoRa y módulo GPS opcional, basado en módulo TTGO LoRa v1.
*/

#include "config.h"
#include "lora_helpers.h"
#include "bt_helpers.h"
#include "tasks_helpers.h"
#include "main.h"

void setup() {
	config::init_board();
	config::init_nvs();
	// config::set_parameters(3000, 1500, 500, 60, 0, 200, 100, 200, -350, 250, 600, 0);
	config::get_parameters();

	tasks_helpers::init_blink();
	
	// if (!digitalRead(PIN_START_BTN)) {
	// 	config::init_bluetooth(BT_NAME);
	// 	config::blink_rate = BLINK_RATE_CONFIG;
	// } else {
	// 	config::mode = config::LISTO;
	// }
	
	if (!digitalRead(PIN_HOOK_TOP)) { // Modo config
		config::init_bluetooth(BT_NAME);
		config::blink_rate = BLINK_RATE_CONFIG;
	} else { // Modo vuelo
		config::blink_rate = BLINK_RATE_ARMED;
		config::mode = config::CIRCULANDO;
		config::command_processed = false;
		config::flight_started = true;
	}
	
	// config::show_oled(0, 0, "OK", false);
}


void loop() {
	static LoraController lora_ctrl{SCK, MISO, MOSI, SS, RST, DIO0};
	
	if (config::bt_enabled && !config::flight_started) {
		if (config::client_just_connected || config::notify_params_saved || config::send_current_params) bt_helpers::bt_send_params();
		bt_helpers::bt_out_ctrl();
		bt_helpers::bt_in_ctrl();

		if (config::reset_blink && millis() - config::blink_timer >= RESET_DELAY) {
			config::blink_rate = BLINK_RATE_READY;
			config::reset_blink = false;
		}
	} else {
		// if (config::mode == config::LISTO) {
		// 	main::startbtn_control();
		// } else {
			if (config::mode != config::VUELO && config::mode != config::DESTERMALIZADO) main::endstops_control();
			if (lora_ctrl.activate()) {
				config::mode = config::DESTERMALIZADO;
				config::command_processed = false;
				
				ESP_LOGI(TAG, "RDT activado (%i)", LoRa.packetRssi());
			}
		// }

		main::modes_control();
	}
}
