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

#define TDAaddress 0x44  //!< I2C address
#define ACK_CHECK_EN 0x1 /*!< I2C master will check ack from slave*/

#define STORAGE_NAMESPACE "TDA7313"

typedef enum input_mode_t {
	COMPUTER = 1,
	RADIO,
	BLUETOOTH
} input_mode_t;

struct tda_settings
{
	uint8_t cleared; // 0xAABB if initialized
	uint8_t Input;
	uint8_t Sla[3];
	uint8_t Volume;
	uint8_t Bass;
	uint8_t Treble;
	uint8_t AttLF;
	uint8_t AttRF;
	uint8_t AttLR;
	uint8_t AttRR;
	bool Mute;
	bool Loud[3];
	bool rear_on;
	bool present;
} Tda_Settings;
extern struct tda_settings TDA;

i2c_cmd_handle_t hnd; // Handle for driver
uint8_t addressTDA;
esp_err_t tda7313_command(uint8_t cmd);
esp_err_t tda7313_detect(uint8_t address);

/**
 * Set SLA
 * @param arg sla 0,1,2,3      0dB, +3.75dB, +7.5dB, +11.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_sla(uint8_t arg);

/**
 * Get SLA
 *  @param input 1,2,3
 * @return SLA for audio input
 */
uint8_t tda7313_get_sla(uint8_t input);

/**
 * Initialize device descriptor
 * @param dev Device descriptor
 * @param port I2C port number
 * @param sda_gpio GPIO pin number for SDA
 * @param scl_gpio GPIO pin number for SCL
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_init();

/**
 * Initialize device setting
 * @param bool erase - forced erasing of NVS if true
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_init_nvs(bool erase);

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
 * Get Mute
  * @return state Mute
 */
bool tda7313_get_mute();

/**
 * Set Loud
 * @param bool loudEnabled false or true
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_loud(bool loudEnabled);

/**
 * Get Loud
 * @param input 1,2,3
 * @return OFF\ON Loudness for audio input
 */
bool tda7313_get_loud(uint8_t input);

/**
 * Switch input
 * @param arg Input 1,2,3    Stereo 1, Stereo 2, Stereo 3
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_input(uint8_t arg);

/**
 * Get audio input
  * @return current audio input
 */
uint8_t tda7313_get_input();

/**
 * Get chip present
  * @return chip present
 */
bool tda7313_get_present();

/**
 * Set master volume
 * @param arg  0.......17    -78.75dB............0dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_volume(uint8_t arg);

/**
 * Get volume
  * @return current volume value
 */
uint8_t tda7313_get_volume();

/**
 * Set Bass
 * @param arg  0....7...14  -14dB.....0dB....+14dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_bass(uint8_t arg);

/**
 * Get Bass
 * @return current Bass value
 */
uint8_t tda7313_get_bass();

/**
 * Set Treble
 * @param arg  0....7...14  -14dB.....0dB....+14dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_treble(uint8_t arg);

/**
 * Get Treble
 * @return Treble
 */
uint8_t tda7313_get_treble();

/**
 * Set Attenuator Left Front Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attlf(uint8_t arg);

/**
 * Get Attenuator Left Front Audio Channel
  * @return value
 */
uint8_t tda7313_get_attlf();

/**
 * Set Attenuator Right Front Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attrf(uint8_t arg);

/**
 * Get Attenuator Right Front Audio Channel
 * @return value
 */
uint8_t tda7313_get_attrf();

/**
 * Set Attenuator Left Rear Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attlr(uint8_t arg);

/**
 * Get Attenuator Left Rear Audio Channel
 * @return value
 */
uint8_t tda7313_get_attlr();

/**
 * Set Attenuator Right Rear Audio Channel
 * @param arg  0.......13  0dB.........-36.25dB
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_attrr(uint8_t arg);

/**
 * Get Attenuator Right Rear Audio Channel
  * @return value
 */
uint8_t tda7313_get_attrr();

/**
 * Enabling or disabling The Rear speakers.
 * @param rear_on  true = Enabled, false = Disabled the Rear Speakers.
 * @return `ESP_OK` on success
 */
esp_err_t tda7313_set_rear(bool rear_on);

/**
 * Checking Rear speakers are on or off.
  * @return true or false
 */
bool tda7313_get_rear();

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __TDA7313_H__ */
