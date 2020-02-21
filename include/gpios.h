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

//---------------------------------//
// Define GPIO used in ESP32-Radiola //
//---------------------------------//

// Default value, can be superseeded by the hardware partition.

//-------------------------

#define PIN_NUM_PWM GPIO_NUM_13     //пин управления оборотами вентилятора
#define PIN_NUM_TACH GPIO_NUM_12    //Пин контроля оборотов вентилятора
#define PIN_NUM_DS18B20 GPIO_NUM_14 //пин датчика температуры
//#define PIN_NUM_SDCS GPIO_NUM_15    //пин выбора SD карты
#define PIN_NUM_BUZZ GPIO_NUM_33    //пин BUZZER
#define PIN_NUM_LDR GPIO_NUM_39       //пин фоторезистора
#define PIN_NUM_KBD GPIO_NUM_34       //пин клавиатуры

//
#define PIN_NUM_STB GPIO_NUM_25       //пин STAND BY

// I2C
//------------------------------------------------
#define PIN_I2C_SDA GPIO_NUM_21
#define PIN_I2C_SCL GPIO_NUM_22

// SOFTUART
//------------------------------------------------
#define PIN_NUM_RXD GPIO_NUM_36 // RXD BT201
#define PIN_NUM_TXD GPIO_NUM_15 // TXD BT201
//-------------------------
// Must be HSPI or VSPI

#define KSPI HSPI_HOST

// KSPI pins of the SPI bus
//-------------------------
#define PIN_NUM_MISO GPIO_NUM_19 // Master Input, Slave Output
#define PIN_NUM_MOSI GPIO_NUM_23 // Master Output, Slave Input   Named Data or SDA or D1 for oled
#define PIN_NUM_CLK GPIO_NUM_18  // Master clock  Named SCL or SCK or D0 for oled

// gpio of the vs1053
//-------------------
#define PIN_NUM_XCS GPIO_NUM_5
#define PIN_NUM_XDCS GPIO_NUM_32
#define PIN_NUM_DREQ GPIO_NUM_4
// + KSPI pins

// Encoder knob
//-------------
#define PIN_ENC0_A GPIO_NONE   //16	// 255 if encoder not used
#define PIN_ENC0_B GPIO_NONE   //17	// DT
#define PIN_ENC0_BTN GPIO_NONE //5// SW

// SPI lcd
//---------
#define PIN_LCD_CS GPIO_NUM_27 //CS
#define PIN_LCD_A0 GPIO_NUM_2  //A0 or D/C

// IR Signal
//-----------
#define PIN_IR_SIGNAL GPIO_NUM_35 // Remote IR source

// LCD backlight control
#define PIN_LCD_BACKLIGHT GPIO_NUM_26 // the gpio to be used in custom.c

// touch screen  T_DO is MISO, T_DIN is MOSI, T_CLK is CLk of the spi bus
#define PIN_TOUCH_CS GPIO_NUM_0 //Chip select T_CS

// init a gpio as output
//void gpio_output_conf(gpio_num_t gpio);

// get the hardware partition infos
esp_err_t open_partition(const char *partition_label, const char *namespace, nvs_open_mode open_mode, nvs_handle *handle);
void close_partition(nvs_handle handle, const char *partition_label);
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
esp_err_t gpio_get_touch(gpio_num_t *touch);
esp_err_t gpio_set_touch(gpio_num_t touch);
void option_get_lcd_rotat(uint8_t *rt);
void option_set_lcd_rotat(uint8_t rt);
void option_get_ddmm(uint8_t *ddmm);
void option_set_ddmm(uint8_t ddmm);
bool option_get_gpio_mode();
void option_set_gpio_mode(uint8_t gpio_mode);
void option_get_lcd_out(uint32_t *lcd_out);
void option_set_lcd_out(uint32_t lcd_out);
esp_err_t gpio_get_ds18b20(gpio_num_t *ds18b20);
esp_err_t gpio_set_ds18b20(gpio_num_t ds18b20);
esp_err_t gpio_get_fanspeed(gpio_num_t *fanspeed);
esp_err_t gpio_set_fanspeed(gpio_num_t fanspeed);
esp_err_t gpio_get_buzzer(gpio_num_t *buzzer);
esp_err_t gpio_set_buzzer(gpio_num_t buzzer);
esp_err_t gpio_get_ldr(gpio_num_t *ldr);
esp_err_t gpio_set_ldr(gpio_num_t ldr);

#endif
