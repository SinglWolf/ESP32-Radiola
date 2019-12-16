/**
 * @file tda7313.c
 *
 * ESP-IDF driver for TDA7313 audioprocessors
 *
 * Copyright (C) 2019 Alex Wolf <singlwolf@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <string.h>
#include <esp_log.h>
#include "tda7313.h"

static const char *TAG = "TDA7313";

nvs_handle tda_nvs;
typedef unsigned char byte;

static byte iSelector = 0x5F;
static const byte VOLUME_MASK[18] = { 0x3F, 0x2D, 0x26, 0x21, 0x1D, 0x18, 0x14, 0x12, 0x0F, 0x0D, 0x0A, 0x08, 0x06, 0x05, 0x04, 0x03, 0x01, 0x00 };
static const byte BASS_MASK[15] = { 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68 };
static const byte TREBLE_MASK[15] = { 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78 };
static const byte ATT_LF_MASK[14] = { 0x80, 0x81, 0x83, 0x84, 0x85, 0x86, 0x88, 0x8A, 0x8D, 0x8F, 0x92, 0x94, 0x98, 0x9D };
static const byte ATT_RF_MASK[14] = { 0xA0, 0xA1, 0xA3, 0xA4, 0xA5, 0xA6, 0xA8, 0xAA, 0xAD, 0xAF, 0xB2, 0xB4, 0xB8, 0xBD };
static const byte ATT_LR_MASK[14] = { 0xC0, 0xC1, 0xC3, 0xC4, 0xC5, 0xC6, 0xC8, 0xCA, 0xCD, 0xCF, 0xD2, 0xD4, 0xD8, 0xDD };
static const byte ATT_RR_MASK[14] = { 0xE0, 0xE1, 0xE3, 0xE4, 0xE5, 0xE6, 0xE8, 0xEA, 0xED, 0xEF, 0xF2, 0xF4, 0xF8, 0xFD };

static const float VOLUME_PRINT[18] = { -78.75, -56.25, -47.50, -41.25, -36.25, -30.00, -25.00, -22.50, -18.75, -16.25, -12.50, -10.00, -7.50, -6.25, -5.00,
		-3.75, -1.25, 0 };
static const int BASS_TREBLE_PRINT[15] = { -14, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14 };
static const float ATT_PRINT[14] = { 0, -1.25, -3.75, -5, -6.25, -7.5, -10, -12.5, -16.25, -18.75, -22.5, -25, -30, -36.25 };
static const float SLA_PRINT[4] = { 0, 3.75, 7.5, 11.25 };

struct tda_settings TDA;

esp_err_t tda7313_set_sla(uint8_t arg) {

	switch (arg) {
	case 0:
		iSelector |= (1 << 3);
		iSelector |= (1 << 4);
		TDA.iSla[TDA.iInput - 1] = arg;
		break;
	case 1:
		iSelector &= ~(1 << 3);
		iSelector |= (1 << 4);
		TDA.iSla[TDA.iInput - 1] = arg;
		break;
	case 2:
		iSelector |= (1 << 3);
		iSelector &= ~(1 << 4);
		TDA.iSla[TDA.iInput - 1] = arg;
		break;
	case 3:
		iSelector &= ~(1 << 3);
		iSelector &= ~(1 << 4);
		TDA.iSla[TDA.iInput - 1] = arg;
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "SLA: %.2f dB", SLA_PRINT[arg]);

	return tda7313_command(iSelector);
}

esp_err_t tda7313_set_loud(bool loudEnabled) {

	if (loudEnabled)
		iSelector &= ~(1 << 2);
	else
		iSelector |= (1 << 2);
	TDA.iLoud = loudEnabled;

	return tda7313_command(iSelector);
}

esp_err_t tda7313_set_input(uint8_t arg) {

	switch (arg) {
	case 1:
		iSelector &= ~(1 << 0);
		iSelector &= ~(1 << 1);
		TDA.iInput = arg;
		break;
	case 2:
		iSelector |= (1 << 0);
		iSelector &= ~(1 << 1);
		TDA.iInput = arg;
		break;
	case 3:
		iSelector &= ~(1 << 0);
		iSelector |= (1 << 1);
		TDA.iInput = arg;
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}

	ESP_ERROR_CHECK(tda7313_command(iSelector));

	ESP_LOGD(TAG, "Input: %d", arg);
	return tda7313_set_sla(TDA.iSla[arg - 1]);
}

esp_err_t tda7313_set_mute(bool muteEnabled) {
	esp_err_t err;
	if (muteEnabled) {
		TDA.iMute = true;
		err = tda7313_command(0x9F);
		err = tda7313_command(0xBF);
		err = tda7313_command(0xDF);
		err = tda7313_command(0xFF);
		ESP_LOGD(TAG, "Mute Enabled");
	} else {
		TDA.iMute = false;
		err = tda7313_set_attlf(TDA.iAttLF);
		err = tda7313_set_attrf(TDA.iAttRF);
		err = tda7313_set_attlr(TDA.iAttLR);
		err = tda7313_set_attrr(TDA.iAttRR);
		ESP_LOGD(TAG, "Mute Disabled");
	}

	return err;
}

esp_err_t tda7313_set_volume(int8_t arg) {
	if ((arg < 0) || (arg > 17)) {
		ESP_LOGE(TAG, "Volume value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	if (TDA.iMute)
		ESP_ERROR_CHECK(tda7313_set_mute(false));

	TDA.iVolume = arg;

	ESP_LOGD(TAG, "Volume: %.2f dB", VOLUME_PRINT[arg]);

	return tda7313_command(VOLUME_MASK[TDA.iVolume]);
}

esp_err_t tda7313_set_bass(int8_t arg) {
	if ((arg < 0) || (arg > 14)) {
		ESP_LOGE(TAG, "Bass value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	TDA.iBass = arg;

	ESP_LOGD(TAG, "Bass: %d dB", BASS_TREBLE_PRINT[arg]);

	return tda7313_command(BASS_MASK[TDA.iBass]);
}

esp_err_t tda7313_set_treble(int8_t arg) {
	if ((arg < 0) || (arg > 14)) {
		ESP_LOGE(TAG, "Treble value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	TDA.iTreble = arg;

	ESP_LOGD(TAG, "Treble: %d dB", BASS_TREBLE_PRINT[arg]);

	return tda7313_command(TREBLE_MASK[TDA.iTreble]);
}

esp_err_t tda7313_set_attlf(int8_t arg) {
	if ((arg < 0) || (arg > 13)) {
		ESP_LOGE(TAG, "ATT_LF value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	if (TDA.iMute) {
		ESP_LOGD(TAG, "Mute Enabled");
		return ESP_OK;
	}
	TDA.iAttLF = arg;

	ESP_LOGD(TAG, "ATT_LF: %.2f dB", ATT_PRINT[arg]);

	return tda7313_command(ATT_LF_MASK[TDA.iAttLF]);
}

esp_err_t tda7313_set_attrf(int8_t arg) {
	if ((arg < 0) || (arg > 13)) {
		ESP_LOGE(TAG, "ATT_RF value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	if (TDA.iMute) {
		ESP_LOGD(TAG, "Mute Enabled");
		return ESP_OK;
	}
	TDA.iAttRF = arg;

	ESP_LOGD(TAG, "ATT_RF: %.2f dB", ATT_PRINT[arg]);

	return tda7313_command(ATT_RF_MASK[TDA.iAttRF]);
}

esp_err_t tda7313_set_attlr(int8_t arg) {
	if ((arg < 0) || (arg > 13)) {
		ESP_LOGE(TAG, "ATT_LR value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	if (TDA.iMute) {
		ESP_LOGD(TAG, "Mute Enabled");
		return ESP_OK;
	}
	TDA.iAttLR = arg;

	ESP_LOGD(TAG, "ATT_LR: %.2f dB", ATT_PRINT[arg]);

	return tda7313_command(ATT_LR_MASK[TDA.iAttLR]);
}

esp_err_t tda7313_set_attrr(int8_t arg) {
	if ((arg < 0) || (arg > 13)) {
		ESP_LOGE(TAG, "ATT_RR value: %d wrong!", arg);
		return ESP_ERR_INVALID_ARG;
	}

	if (TDA.iMute) {
		ESP_LOGD(TAG, "Mute Enabled");
		return ESP_OK;
	}
	TDA.iAttRR = arg;

	ESP_LOGD(TAG, "ATT_RR: %.2f dB", ATT_PRINT[arg]);

	return tda7313_command(ATT_RR_MASK[TDA.iAttRR]);
}

esp_err_t tda7313_init(gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
	esp_err_t err;
	addressTDA = TDAaddress;
	if ((sda_gpio >= 0) || // TDA configured?
			(scl_gpio >= 0)) {
		ESP_LOGI(TAG, "Init I2C driver, pins %d, %d", sda_gpio, scl_gpio);
		i2c_config_t i2c_config; // I2C configuration
		i2c_config.mode = I2C_MODE_MASTER, i2c_config.sda_io_num = sda_gpio;
		i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
		i2c_config.scl_io_num = scl_gpio;
		i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
		i2c_config.master.clk_speed = 100000;
		err = i2c_param_config(I2C_NUM_0, &i2c_config);
		if (err == ESP_OK) {
			err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
			if (err == ESP_OK) {
				ESP_LOGI(TAG, "Init I2C driver OK.");
				ESP_LOGI(TAG, "Start TDA7313");

				if (tda7313_detect(TDAaddress) == ESP_OK) {
					return tda7313_init_nvs();
				} else {
					return err;
				}
			}
		} else {
			ESP_LOGE(TAG, "Init I2C driver FAILED.");
			return err;
		}
	} else {
		ESP_LOGI(TAG, "TDA7313 not configured.");
		err = ESP_OK;
	}
	return err;

}
esp_err_t tda7313_init_nvs() {
	esp_err_t err;
	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
	// Open
	ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &tda_nvs));
//
	err = nvs_get_u16(tda_nvs, "tda_cleared", &TDA.cleared);
	if (err == ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGI(TAG, "Keys for the TDA7313 in NVS not founds.");
		ESP_LOGI(TAG, "Creating default values and saving in new keys.");
		TDA.cleared = 0xAABB; 		// 0xAABB if initialized
		ESP_ERROR_CHECK(nvs_set_u16(tda_nvs, "tda_cleared", TDA.cleared));
		TDA.iVolume = 9;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_volume", TDA.iVolume));
		TDA.iBass = 7;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_bass", TDA.iBass));
		TDA.iTreble = 7;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_treble", TDA.iTreble));
		TDA.iAttLF = 0;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attlf", TDA.iAttLF));
		TDA.iAttRF = 0;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attrf", TDA.iAttRF));
		TDA.iAttLR = 13;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attlr", TDA.iAttLR));
		TDA.iAttRR = 13;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attrr", TDA.iAttRR));
		TDA.iInput = 2;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_input", TDA.iInput));
		TDA.iSla[0] = 0;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_sla1", TDA.iSla[0]));
		TDA.iSla[1] = 0;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_sla2", TDA.iSla[1]));
		TDA.iSla[2] = 0;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_sla3", TDA.iSla[2]));
		TDA.iMute = false;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_mute", 0));
		TDA.iLoud = false;
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_loud", 0));
		ESP_ERROR_CHECK(nvs_commit(tda_nvs));
	} else {
		int8_t temp;
		ESP_LOGI(TAG, "Keys for the TDA7313 in NVS founds.");
		ESP_LOGI(TAG, "Loading values from keys to chip.");
//		ESP_ERROR_CHECK(nvs_get_u16(tda_nvs, "tda_cleared", &TDA.cleared));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_volume", &TDA.iVolume));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_bass", &TDA.iBass));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_treble", &TDA.iTreble));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attlf", &TDA.iAttLF));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attrf", &TDA.iAttRF));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attlr", &TDA.iAttLR));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attrr", &TDA.iAttRR));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_input", &TDA.iInput));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_sla1", &TDA.iSla[0]));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_sla2", &TDA.iSla[1]));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_sla3", &TDA.iSla[2]));
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_mute", &temp));
		TDA.iMute = (temp == 0 ? false : true);
		ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_loud", &temp));
		TDA.iLoud = (temp == 0 ? false : true);
	}
	nvs_close(tda_nvs);
	ESP_ERROR_CHECK(tda7313_set_volume(TDA.iVolume));
	ESP_ERROR_CHECK(tda7313_set_treble(TDA.iTreble));
	ESP_ERROR_CHECK(tda7313_set_bass(TDA.iBass));
	ESP_ERROR_CHECK(tda7313_set_attlf(TDA.iAttLF));
	ESP_ERROR_CHECK(tda7313_set_attrf(TDA.iAttRF));
	ESP_ERROR_CHECK(tda7313_set_attlr(TDA.iAttLR));
	ESP_ERROR_CHECK(tda7313_set_attrr(TDA.iAttRR));
	ESP_ERROR_CHECK(tda7313_set_input(TDA.iInput));
	ESP_ERROR_CHECK(tda7313_set_mute(TDA.iMute));
	ESP_ERROR_CHECK(tda7313_set_loud(TDA.iLoud));

	if ((TDA.iSla[0] < 0) || (TDA.iSla[0] > 4))
		TDA.iSla[0] = 0;
	if ((TDA.iSla[1] < 0) || (TDA.iSla[1] > 4))
		TDA.iSla[1] = 0;
	if ((TDA.iSla[2] < 0) || (TDA.iSla[2] > 4))
		TDA.iSla[2] = 0;

	return tda7313_set_sla(TDA.iSla[TDA.iInput - 1]);

}
esp_err_t tda7313_save_nvs() {
	esp_err_t err = ESP_OK;
	int8_t temp;
	ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &tda_nvs));
//
	ESP_LOGI(TAG, "Keys for the TDA7313 keep to NVS.");
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_volume", &temp));
	if (TDA.iVolume != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_volume", TDA.iVolume));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_bass", &temp));
	if (TDA.iBass != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_bass", TDA.iBass));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_treble", &temp));
	if (TDA.iTreble != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_treble", TDA.iTreble));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attlf", &temp));
	if (TDA.iAttLF != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attlf", TDA.iAttLF));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attrf", &temp));
	if (TDA.iAttRF != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attrf", TDA.iAttRF));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attlr", &temp));
	if (TDA.iAttLR != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attlr", TDA.iAttLR));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_attrr", &temp));
	if (TDA.iAttRR != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_attrr", TDA.iAttRR));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_input", &temp));
	if (TDA.iInput != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_input", TDA.iInput));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_sla1", &temp));
	if (TDA.iSla[0] != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_sla1", TDA.iSla[0]));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_sla2", &temp));
	if (TDA.iSla[1] != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_sla2", TDA.iSla[1]));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_sla3", &temp));
	if (TDA.iSla[2] != temp)
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_sla3", TDA.iSla[2]));
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_mute", &temp));
	if (TDA.iMute != temp) {
		temp = (TDA.iMute == false ? 0 : 1);
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_mute", temp));
	}
	ESP_ERROR_CHECK(nvs_get_i8(tda_nvs, "tda_loud", &temp));
	if (TDA.iLoud != temp) {
		temp = (TDA.iLoud == false ? 0 : 1);
		ESP_ERROR_CHECK(nvs_set_i8(tda_nvs, "tda_loud", temp));
	}
	ESP_ERROR_CHECK(nvs_commit(tda_nvs));
	nvs_close(tda_nvs);
	return err;
}
esp_err_t tda7313_command(uint8_t cmd) {
	hnd = i2c_cmd_link_create(); // Create a link
	if (i2c_master_start(hnd) | i2c_master_write_byte(hnd, (addressTDA << 1) | // Add I2C address to output buffer
			I2C_MASTER_WRITE, ACK_CHECK_EN) | i2c_master_write_byte(hnd, cmd, ACK_CHECK_EN) | i2c_master_stop(hnd) | // End of data for I2C
			i2c_master_cmd_begin(I2C_NUM_0, hnd, // Send bufferd data to TDA7313
					10 / portTICK_PERIOD_MS)) {
		ESP_LOGE(TAG, "TDA7313 communication error!"); // Something went wrong (not connected)
		return ESP_FAIL;
	}
	i2c_cmd_link_delete(hnd); // Ссылка больше не нужна
	return ESP_OK;
}
esp_err_t tda7313_detect(uint8_t address) {
	esp_err_t err;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "Device at address %02X found", address);
	} else if (err == ESP_ERR_TIMEOUT) {
		ESP_LOGE(TAG, "Device at address %02X is not responding!", address);
	} else {
		ESP_LOGE(TAG, "Device at address %02X NOT found!", address);
	}
	return err;
}
