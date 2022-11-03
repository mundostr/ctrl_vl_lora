#pragma once

#include "config.h"

class LoraController {
	public:
		LoraController(int sck, int miso, int mosi, int ss, int rst, int dio0) {
			SPI.begin(sck, miso, mosi, ss);
			LoRa.setPins(ss, rst, dio0);
			
			if (!LoRa.begin(LORA_BAND)) {
				ESP_LOGI(TAG, "Lora: ERR");
				for(;;);
			}
		}
	
		bool activate() {
			static String LoRaData;

			if (LoRa.parsePacket()) {
				while (LoRa.available()) { LoRaData = LoRa.readString(); }

				return LoRaData == RDT_COMMAND;
			}

			return false;
		}
};
