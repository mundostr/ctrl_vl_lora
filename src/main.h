#pragma once

#include "config.h"

namespace main {
	void start_sleep(void) {
		esp_deep_sleep_start();
	}

	void adjust_servo(int servo, int pulse) {
		int total_pulse = pulse;
		
		switch (servo) {
			case 1: { // Servo estabilizador
				pulse > 0 ? total_pulse = PWM_MED + config::parameters.offset_estab + pulse : total_pulse = PWM_MED - config::parameters.offset_estab + pulse;
				break;
			}

			default: {
				break;
			}
		}
		
		servo_stab.writeMicroseconds(total_pulse);
	}

	void verify_and_adjust_stab_servo(int pulse, String message, config::modes next_mode, uint32_t time_interval) {
		if (!config::command_processed) {
			config::command_processed = true;

			if (config::parameters.estab_invert == 1) pulse = -pulse;
			adjust_servo(SERVO_STAB_ID, pulse);

			if (next_mode != config::IDLE) {
				config::change_mode_timer_enabled = true;
				config::change_mode_interval = time_interval;
				config::next_mode = next_mode;
				config::change_mode_timer = millis();
			}

			ESP_LOGI(TAG, "Mensaje: %s, pulso: %i, intervalo: %i", message, pulse, time_interval);
		}
	}

	void startbtn_control() {
		start_btn.update();

		// if (start_btn.fell() && !config::flight_started) {
		if ( start_btn.read() == LOW && start_btn.currentDuration() > START_LONGPRESS_PERIOD && !config::flight_started) {
			config::blink_rate = BLINK_RATE_ARMED;
			config::mode = config::CIRCULANDO;
			config::command_processed = false;
			config::flight_started = true;
		}
	}

	void endstops_control() {
		endstop_top.update();
		endstop_front.update();
			
		// Remolque
		if (digitalRead(PIN_HOOK_TOP) && endstop_front.fell() && config::mode != config::REMOLCANDO) {
			config::mode = config::REMOLCANDO;
			config::command_processed = false;
		}

		// Circular
		if (digitalRead(PIN_HOOK_TOP) && endstop_front.rose() && config::mode != config::CIRCULANDO) {
			config::mode = config::CIRCULANDO;
			config::command_processed = false;
		}

		// Despegue
		if (endstop_top.fell() && !digitalRead(PIN_HOOK_FRONT) && config::mode != config::DESPEGUE) {
			config::mode = config::DESPEGUE;
			config::command_processed = false;
		}

		// Trepada
		if (!digitalRead(PIN_HOOK_TOP) && digitalRead(PIN_HOOK_FRONT) && config::mode == config::DESPEGUE) {
			config::mode = config::TREPADA;
			config::command_processed = false;
		}
	}

	void modes_control() {
		switch(config::mode) {
			case config::LISTO:
				// Modo en espera por defecto
				if (config::current_pulse_servo_stab != PWM_MED) {
					config::current_pulse_servo_stab = PWM_MED;
					adjust_servo(SERVO_STAB_ID, 0);
					
					ESP_LOGI(TAG, "LISTO");
				}
				
				break;
			
			case config::REMOLCANDO:
				// Se dispara cuando el gancho está cerrado va hacia adelante
				verify_and_adjust_stab_servo(config::parameters.ang_remolque, "REMOLCANDO", config::IDLE, 0);
				
				break;
			
			case config::CIRCULANDO:
				// Se dispara cuando el gancho está cerrado y va hacia atrás
				verify_and_adjust_stab_servo(config::parameters.ang_circular, "CIRCULANDO", config::IDLE, 0);
				
				break;
			
			case config::DESPEGUE:
				// Se dispara cuando el gancho está abierto y va hacia atrás
				verify_and_adjust_stab_servo(config::parameters.ang_despegue, "DESPEGUE", config::IDLE, 0);
				
				break;
			
			case config::TREPADA:
				// Se dispara de forma automática por tiempo
				verify_and_adjust_stab_servo(config::parameters.ang_vuelo, "TREPADA", config::TRANSICION, config::parameters.tpo_trepada);
				
				break;
			
			case config::TRANSICION:
				// Se dispara de forma automática por tiempo
				verify_and_adjust_stab_servo(config::parameters.ang_transicion, "TRANSICION", config::VUELO, config::parameters.tpo_transicion);
				
				break;
			
			case config::VUELO:
				// Se dispara de forma automática por tiempo
				verify_and_adjust_stab_servo(config::parameters.ang_vuelo, "VUELO", config::DESTERMALIZADO, config::parameters.tpo_vuelo * 1000);
				// verify_and_adjust_stab_servo(config::parameters.ang_vuelo, "VUELO", config::DESTERMALIZADO, 5 * 1000);

				break;
			
			case config::DESTERMALIZADO:
				// Se dispara de forma automática por tiempo
				verify_and_adjust_stab_servo(config::parameters.ang_dt, "DESTERMALIZADO", config::DETENIDO, WAIT_BEFORE_SUSPEND_PERIOD);

				break;
			
			case config::DETENIDO:
				// Se dispara de forma automática por tiempo o manual por RDT
				ESP_LOGI(TAG, "DEEPSLEEP");

				vTaskDelete(handle_blink);
				esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
				esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
				esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
				esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
				esp_deep_sleep_start();
				
				break;
			
			case config::IDLE:
				
				break;
		}

		if (config::change_mode_timer_enabled && millis() - config::change_mode_timer >= config::change_mode_interval) {
			config::mode = config::next_mode;
			config::command_processed = false;
			config::change_mode_timer_enabled = false;
		}
	}
}
