/**
 * @file tda7313.h
 * @defgroup tda7313 tda7313
 * @{
 *
 * ESP-IDF driver for TDA7313 audioprocessors
 *
 * Copyright (C) 2019 Alex Wolf <singlwolf@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */
#ifndef __TDA7313_H__
#define __TDA7313_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <esp_err.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "driver/gpio.h"
#include <driver/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TDAaddress 0x44 //!< I2C address
#define ACK_CHECK_EN 0x1 /*!< I2C master will check ack from slave*/

#define STORAGE_NAMESPACE "TDA7313"

struct tda_settings {
	uint16_t cleared; 		// 0xAABB if initialized
	uint8_t iInput;
	uint8_t iSla[3];		
	uint8_t iVolume;
	uint8_t iBass;
	uint8_t iTreble;
	uint8_t iAttLF;
	uint8_t iAttRF;
	uint8_t iAttLR;
	uint8_t iAttRR;
	bool iMute;
	bool iLoud;
} Tda_Settings;
extern struct tda_settings TDA;

i2c_cmd_handle_t hnd; // Handle for driver
uint8_t addressTDA;
esp_err_t  tda7313_command(uint8_t cmd);
esp_err_t tda7313_detect(uint8_t address);

/**
 * Set SLA
 * @param arg sla 0,1,2,3      0dB, +3.75dB, +7.5dB, +11.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_sla(uint8_t arg);

/**
 * Initialize device descriptor
 * @param dev Device descriptor
 * @param port I2C port number
 * @param sda_gpio GPIO pin number for SDA
 * @param scl_gpio GPIO pin number for SCL
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_init(gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * Initialize device setting
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_init_nvs();

/**
 * Save device setting to NVS
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_save_nvs();

/**
 * Set Mute
 * @param bool muteEnabled false or true
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_mute(bool muteEnabled);

/**
 * Set Loud
 * @param bool loudEnabled false or true
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_loud(bool loudEnabled);

/**
 * Switch input
 * @param arg Input 1,2,3    Stereo 1, Stereo 2, Stereo 3
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_input(uint8_t arg);

/**
 * Set master volume
 * @param arg  0.......17    -78.75dB............0dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_volume(uint8_t arg);

/**
 * Set Bass
 * @param arg  0....7...14  -14dB.....0dB....+14dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_bass(uint8_t arg);
/**
 * Set Treble
 * @param arg  0....7...14  -14dB.....0dB....+14dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_treble(uint8_t arg);

/**
 * Set Attenuator Left Front Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attlf(uint8_t arg);

/**
 * Set Attenuator Right Front Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attrf(uint8_t arg);

/**
 * Set Attenuator Left Rear Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attlr(uint8_t arg);

/**
 * Set Attenuator Right Rear Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attrr(uint8_t arg);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __TDA7313_H__ */
