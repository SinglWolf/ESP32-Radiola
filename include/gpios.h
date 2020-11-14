/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/
#pragma once
#ifndef __GPIOS_H__
#define __GPIOS_H__

#include "nvs_flash.h"
#include "driver/spi_master.h"
#include "esp_adc_cal.h"

#define GPIO_NONE 255

//-------------------------------------------------------------//
// Назначение GPIOs, используемых в ESP32-Radiola по-умолчанию //
//-------------------------------------------------------------//

//-------------------------
#ifdef CONFIG_COOLER_PRESENT
#define PIN_NUM_PWM CONFIG_PWM_GPIO   //пин управления оборотами вентилятора
#define PIN_NUM_TACH CONFIG_TACH_GPIO //Пин контроля оборотов вентилятора
#else
#define PIN_NUM_PWM GPIO_NONE
#define PIN_NUM_TACH GPIO_NONE
#endif
#ifdef CONFIG_TEMPERATURE_SENSOR_PRESENT
#define PIN_NUM_DS18B20 CONFIG_DS18B20_GPIO //пин датчика температуры
#else
#define PIN_NUM_DS18B20 GPIO_NONE
#endif
#ifdef CONFIG_BUZZER_PRESENT
#define PIN_NUM_BUZZ CONFIG_GPIO_BUZZ //пин BUZZER
#else
#define PIN_NUM_BUZZ GPIO_NONE
#endif
#ifdef CONFIG_LDR_SENSOR_PRESENT
#define PIN_NUM_LDR CONFIG_GPIO_LDR //пин фоторезистора
#else
#define PIN_NUM_LDR GPIO_NONE
#endif
#ifdef CONFIG_TYPE_INPUT_KEYBOARD
#define PIN_NUM_KBD CONFIG_KBD_GPIO
#else
#define PIN_NUM_KBD GPIO_NONE //пин клавиатуры GPIO_NUM_34 // 255 GPIO_NONE (255), если не используется
#endif
#ifdef CONFIG_AMPLIFIER_CONTROL_PRESENT
#define PIN_NUM_STB CONFIG_GPIO_STB //пин STAND BY
#else
#define PIN_NUM_STB GPIO_NONE
#endif
// I2C
#ifdef CONFIG_TDA7313_PRESENT
#define PIN_I2C_SDA CONFIG_I2C_SDA_GPIO
#define PIN_I2C_SCL CONFIG_I2C_SCL_GPIO
#else
#define PIN_I2C_SDA GPIO_NONE
#define PIN_I2C_SCL GPIO_NONE
#endif
// SOFTUART
#ifdef CONFIG_BT201_PRESENT
#define PIN_NUM_RXD CONFIG_RXD_GPIO // RXD BT201
#define PIN_NUM_TXD CONFIG_TXD_GPIO // TXD BT201
#else
#define PIN_NUM_RXD GPIO_NONE
#define PIN_NUM_TXD GPIO_NONE
#endif
// KSPI
//-------------------------
#ifdef CONFIG_HARDWARE_HOST
#define KSPI HSPI_HOST // HSPI
#else
#define KSPI VSPI_HOST // VSPI
#endif

// Энкодер
#ifdef CONFIG_TYPE_INPUT_ENCODER
#define PIN_ENC_A CONFIG_ENC_A_GPIO     //14
#define PIN_ENC_B CONFIG_ENC_B_GPIO     //13
#define PIN_ENC_BTN CONFIG_ENC_BTN_GPIO //34
#else
#define PIN_ENC_A GPIO_NONE
#define PIN_ENC_B GPIO_NONE
#define PIN_ENC_BTN GPIO_NONE
#endif
// Пульт ДУ
#ifdef CONFIG_IR_SENSOR_PRESENT
#define PIN_IR_SIGNAL CONFIG_IR_SIG_GPIO // Remote IR source
#else
#define PIN_IR_SIGNAL GPIO_NONE // Remote IR source
#endif
// Тачскрин
#ifdef CONFIG_TOUCH_SENSOR_PRESENT
#define PIN_TOUCH_CS CONFIG_TOUCH_GPIO
#else
#define PIN_TOUCH_CS GPIO_NONE //Chip select T_CS
#endif

esp_err_t gpio_set_nvs(const char *name_pin, gpio_num_t gpio_num);
esp_err_t gpio_get_nvs(const char *name_pin, gpio_num_t *gpio_num);
esp_err_t open_partition(const char *partition_label, const char *namespace, nvs_open_mode open_mode, nvs_handle *handle);
esp_err_t gpio_get_spi_bus(uint8_t *spi_no, gpio_num_t *miso, gpio_num_t *mosi, gpio_num_t *sclk);
esp_err_t gpio_set_spi_bus(uint8_t spi_no, gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk);
esp_err_t gpio_get_vs1053(gpio_num_t *xcs, gpio_num_t *xdcs, gpio_num_t *dreq);
esp_err_t gpio_set_vs1053(gpio_num_t xcs, gpio_num_t xdcs, gpio_num_t dreq);
esp_err_t gpio_get_encoders(gpio_num_t *enca, gpio_num_t *encb, gpio_num_t *encbtn);
esp_err_t gpio_set_encoders(gpio_num_t enca, gpio_num_t encb, gpio_num_t encbtn);
esp_err_t gpio_get_i2c(gpio_num_t *sda, gpio_num_t *scl);
esp_err_t gpio_set_i2c(gpio_num_t sda, gpio_num_t scl);
esp_err_t gpio_get_uart(gpio_num_t *rxd, gpio_num_t *txd);
esp_err_t gpio_set_uart(gpio_num_t rxd, gpio_num_t txd);
esp_err_t gpio_get_spi_lcd(gpio_num_t *cs, gpio_num_t *a0);
esp_err_t gpio_set_spi_lcd(gpio_num_t cs, gpio_num_t a0);
esp_err_t gpio_get_ir_signal(gpio_num_t *ir);
esp_err_t gpio_set_ir_signal(gpio_num_t ir);
esp_err_t gpio_get_backlightl(gpio_num_t *led);
esp_err_t gpio_set_backlightl(gpio_num_t led);
esp_err_t gpio_get_tachometer(gpio_num_t *tach);
esp_err_t gpio_set_tachometer(gpio_num_t tach);
esp_err_t gpio_get_ir_key(nvs_handle handle, const char *key, uint32_t *out_set1, uint32_t *out_set2);
esp_err_t gpio_set_ir_key(const char *key, char *ir_keys);
esp_err_t gpio_get_stb(gpio_num_t *stb);
esp_err_t gpio_set_stb(gpio_num_t stb);
esp_err_t gpio_get_touch(gpio_num_t *touch);
esp_err_t gpio_set_touch(gpio_num_t touch);
esp_err_t gpio_get_ds18b20(gpio_num_t *ds18b20);
esp_err_t gpio_set_ds18b20(gpio_num_t ds18b20);
esp_err_t gpio_get_fanspeed(gpio_num_t *fanspeed);
esp_err_t gpio_set_fanspeed(gpio_num_t fanspeed);
esp_err_t gpio_get_buzzer(gpio_num_t *buzzer);
esp_err_t gpio_set_buzzer(gpio_num_t buzzer);
esp_err_t gpio_get_ldr(gpio_num_t *ldr);
esp_err_t gpio_set_ldr(gpio_num_t ldr);
void close_partition(nvs_handle handle, const char *partition_label);
void get_lcd_rotat(uint8_t *rt);
void set_lcd_rotat(uint8_t rt);
void get_ddmm(uint8_t *ddmm);
void set_ddmm(uint8_t ddmm);
bool get_gpio_mode();
void set_gpio_mode(uint8_t gpio_mode);
void get_lcd_out(uint32_t *lcd_out);
void set_lcd_out(uint32_t lcd_out);
void get_bright(uint8_t *bright);
void set_bright(uint8_t bright);
void get_fan(uint8_t *fan);
void set_fan(uint8_t fan);

#endif
