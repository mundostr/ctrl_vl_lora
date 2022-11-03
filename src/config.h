/*
https://randomnerdtutorials.com/esp32-lora-sensor-web-server/
https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
*/

#pragma once

#include <Arduino.h>
#include <esp_log.h>
// #include <SPI.h>
#include <LoRa.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <Servo.h>
#include <Bounce2.h>
#include <esp_sleep.h>
#include <nvs_flash.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth deshabilitado, ejecutar menuconfig!
#endif

#define ONBOARD_LED 2
#define PIN_START_BTN 13
#define PIN_HOOK_FRONT 17
#define PIN_HOOK_TOP 23
#define PIN_SERVO_STAB 25

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define LORA_BAND 915E6

// #define OLED_SDA 4
// #define OLED_SCL 15
// #define OLED_RST 16
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64

#define BT_NAME "CTRL_VL_LORA"
#define QT_PARAMS 12
#define BLINK_RATE_READY 1000
#define BLINK_RATE_CONFIG 3000
#define BLINK_RATE_ARMED 250
#define BLINK_RATE_SAVED 100
#define RESET_DELAY 3000
#define WAIT_BEFORE_SUSPEND_PERIOD 3000
#define START_LONGPRESS_PERIOD 3000
#define PWM_MIN 500 // Pulso mínimo servos (ms)
#define PWM_MED 1500 // Pulso medio servos (ms)
#define PWM_MAX 2500 // Pulso máximo servos (ms)
#define SERVO_STAB_ID 1
#define RDT_COMMAND "dt"

static const char *TAG = "TIMER";

// Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
Servo servo_stab;
BluetoothSerial bt;
Bounce2::Button start_btn = Bounce2::Button();
Bounce2::Button endstop_top = Bounce2::Button();
Bounce2::Button endstop_front = Bounce2::Button();
xTaskHandle handle_blink;
esp_err_t err;

namespace config {
	bool bt_enabled = false;
	bool client_connected = false;
	bool client_just_connected = false;
	bool payload_just_received = false;
	bool notify_params_saved = false;
	bool send_current_params = false;
	bool led_active = false;
	bool reset_blink = false;
	bool flight_started = false;
	bool command_processed = false;
	bool change_mode_timer_enabled = false;
	int blink_rate = BLINK_RATE_READY;
	int change_mode_interval = 0;
	int current_pulse_servo_stab = 0;
	uint32_t blink_timer;
	uint32_t change_mode_timer;

	struct paramsFormat {
		int16_t ret_inicio = 3000;
		int16_t tpo_trepada = 1400;
		int16_t tpo_transicion = 500;
		int16_t tpo_vuelo = 60;
		int16_t offset_estab = 0;
		int16_t ang_remolque = 200;
		int16_t ang_circular = 100;
		int16_t ang_despegue = 200;
		int16_t ang_transicion = -350;
		int16_t ang_vuelo = 250;
		int16_t ang_dt = 600;
		int16_t estab_invert = 0;
	};
	paramsFormat parameters;

	enum modes {
		LISTO,
		REMOLCANDO,
		CIRCULANDO,
		DESPEGUE,
		TREPADA,
		TRANSICION,
		VUELO,
		DESTERMALIZADO,
		DETENIDO,
		IDLE
	}; // modos de vuelo
	modes mode, next_mode;

	void init_board(void) {
		pinMode(ONBOARD_LED, OUTPUT);
		digitalWrite(ONBOARD_LED, LOW);

		servo_stab.attach(PIN_SERVO_STAB);

		start_btn.attach(PIN_START_BTN, INPUT_PULLUP);
		start_btn.interval(5);
		start_btn.setPressedState(LOW);

		endstop_top.attach(PIN_HOOK_TOP, INPUT_PULLUP);
		endstop_top.interval(5);
		endstop_top.setPressedState(LOW);

		endstop_front.attach(PIN_HOOK_FRONT, INPUT_PULLUP);
		endstop_front.interval(5);
		endstop_front.setPressedState(LOW);
		
		// esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BTN, HIGH);
	}

	// void init_oled(int textSize, bool invert = false) {
	// 	pinMode(OLED_RST, OUTPUT);
	// 	digitalWrite(OLED_RST, LOW);
	// 	delay(20);
	// 	digitalWrite(OLED_RST, HIGH);
		
	// 	Wire.begin(OLED_SDA, OLED_SCL);
	// 	if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3c, true, true)) {
	// 		if (DEBUG) Serial.println(F("Error Oled"));
	// 		for(;;);
	// 	}
		
	// 	oled.clearDisplay();
	// 	oled.setTextColor(WHITE);
	// 	oled.setTextSize(textSize);
	// 	oled.invertDisplay(invert);
	// }

	// void show_oled(int x, int y, const char *msg, bool clear) {
	// 	if (clear) oled.clearDisplay();
	// 	oled.setCursor(x, y);
	// 	oled.print(msg);
	// 	oled.display();
	// }

	void bt_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
		if (event == ESP_SPP_SRV_OPEN_EVT) {
			client_connected = true;
			client_just_connected = true;
		}

		if (event == ESP_SPP_CLOSE_EVT) {
			client_connected = false;
		}
	}

	void init_bluetooth(String name) {
		if (bt.begin(name)) {
			bt.register_callback(bt_callback);
			bt_enabled = true;
			digitalWrite(ONBOARD_LED, HIGH);
		} else {
		}
	}

	void init_nvs() {
		err = nvs_flash_init();
		
		if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
			ESP_LOGI(TAG, "Borrando NVS...");
			ESP_ERROR_CHECK(nvs_flash_erase());
			err = nvs_flash_init();
		}
		ESP_ERROR_CHECK(err);
		
		ESP_LOGI(TAG, "NVS inicializado");
	}

	void set_parameters(int retardo_inicio, int tiempo_trepada, int tiempo_transicion, int tiempo_vuelo, int offset_estab, int ang_remolque, int ang_circular, int ang_despegue, int ang_transicion, int ang_vuelo, int ang_dt, bool servo_estab_invertido) {
		nvs_handle nvs_ctrl;

		ESP_ERROR_CHECK(nvs_open(BT_NAME, NVS_READWRITE, &nvs_ctrl));
		
		err = nvs_set_i16(nvs_ctrl, "ret_inicio", retardo_inicio);
		err = nvs_set_i16(nvs_ctrl, "tpo_trepada", tiempo_trepada);
		err = nvs_set_i16(nvs_ctrl, "tpo_transicion", tiempo_transicion);
		err = nvs_set_i16(nvs_ctrl, "tpo_vuelo", tiempo_vuelo);
		err = nvs_set_i16(nvs_ctrl, "offset_estab", offset_estab);
		err = nvs_set_i16(nvs_ctrl, "ang_remolque", ang_remolque);
		err = nvs_set_i16(nvs_ctrl, "ang_circular", ang_circular);
		err = nvs_set_i16(nvs_ctrl, "ang_despegue", ang_despegue);
		err = nvs_set_i16(nvs_ctrl, "ang_transicion", ang_transicion);
		err = nvs_set_i16(nvs_ctrl, "ang_vuelo", ang_vuelo);
		err = nvs_set_i16(nvs_ctrl, "ang_dt", ang_dt);
		err = nvs_set_i16(nvs_ctrl, "estab_invert", servo_estab_invertido);
		
		err = nvs_commit(nvs_ctrl);
		nvs_close(nvs_ctrl);
		
		ESP_LOGI(TAG, "NVS seteado");
	}

	void get_parameters() {
		nvs_handle nvs_ctrl;

		err = nvs_open(BT_NAME, NVS_READONLY, &nvs_ctrl);
		if (err == ESP_OK) {
			err = nvs_get_i16(nvs_ctrl, "ret_inicio", &parameters.ret_inicio);
			err = nvs_get_i16(nvs_ctrl, "tpo_trepada", &parameters.tpo_trepada);
			err = nvs_get_i16(nvs_ctrl, "tpo_transicion", &parameters.tpo_transicion);
			err = nvs_get_i16(nvs_ctrl, "tpo_vuelo", &parameters.tpo_vuelo);
			err = nvs_get_i16(nvs_ctrl, "offset_estab", &parameters.offset_estab);
			err = nvs_get_i16(nvs_ctrl, "ang_remolque", &parameters.ang_remolque);
			err = nvs_get_i16(nvs_ctrl, "ang_circular", &parameters.ang_circular);
			err = nvs_get_i16(nvs_ctrl, "ang_despegue", &parameters.ang_despegue);
			err = nvs_get_i16(nvs_ctrl, "ang_transicion", &parameters.ang_transicion);
			err = nvs_get_i16(nvs_ctrl, "ang_vuelo", &parameters.ang_vuelo);
			err = nvs_get_i16(nvs_ctrl, "ang_dt", &parameters.ang_dt);
			err = nvs_get_i16(nvs_ctrl, "estab_invert", &parameters.estab_invert);

			nvs_close(nvs_ctrl);

			ESP_LOGI(TAG,
				"TIni: %i, TTre: %i, TTra: %i, TVue: %i, AOff: %i, ARem: %i, ACir: %i, ADes: %i, ATra: %i, AVue: %i, ADtm: %i, EInv: %i",
				parameters.ret_inicio,
				parameters.tpo_trepada,
				parameters.tpo_transicion,
				parameters.tpo_vuelo,
				parameters.offset_estab,
				parameters.ang_remolque,
				parameters.ang_circular,
				parameters.ang_despegue,
				parameters.ang_transicion,
				parameters.ang_vuelo,
				parameters.ang_dt,
				parameters.estab_invert
			);
		} else {
			ESP_LOGI(TAG, "ERROR al recuperar parametros");
		}
	}
}
