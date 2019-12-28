/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for EDP32-Media 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/
#pragma once
#ifndef __GPIO_H__
#define __GPIO_H__
#include "nvs_flash.h"
#include "driver/spi_master.h"
#include "esp_adc_cal.h"

#define GPIO_NONE 255

//---------------------------------//
// Define GPIO used in ESP32-Media //
//---------------------------------//

// Default value, can be superseeded by the hardware partition.

//-------------------------

#define PIN_NUM_PWM GPIO_NUM_13     //пин управления оборотами вентилятора
#define PIN_NUM_TACH GPIO_NUM_12    //Пин контроля оборотов вентилятора
#define PIN_NUM_DS18B20 GPIO_NUM_14 //пин датчика температуры
#define PIN_NUM_SDCS GPIO_NUM_15    //пин выбора SD карты

// I2C
//------------------------------------------------
#define PIN_I2C_SDA GPIO_NUM_21
#define PIN_I2C_SCL GPIO_NUM_22
//-------------------------
// Must be HSPI or VSPI

#define KSPI HSPI_HOST

// KSPI pins of the SPI bus
//-------------------------
#define PIN_NUM_MISO GPIO_NUM_19 // Master Input, Slave Output
#define PIN_NUM_MOSI GPIO_NUM_23 // Master Output, Slave Input   Named Data or SDA or D1 for oled
#define PIN_NUM_CLK GPIO_NUM_18  // Master clock  Named SCL or SCK or D0 for oled

// status led if any.
//-------------------
// Set the right one with command sys.led
// GPIO can be changed with command sys.ledgpio("x")
#define GPIO_LED GPIO_NUM_25 // Flashing led or Playing led

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
#define PIN_ENC1_A GPIO_NONE   // 255 if encoder not used
#define PIN_ENC1_B GPIO_NONE   // DT
#define PIN_ENC1_BTN GPIO_NONE // SW

// SPI lcd
//---------
#define PIN_LCD_CS GPIO_NUM_27 //CS
#define PIN_LCD_A0 GPIO_NUM_2 //A0 or D/C
#define PIN_LCD_RST GPIO_NONE  //Reset RES RST or not used
// KSPI pins +

// IR Signal
//-----------
#define PIN_IR_SIGNAL GPIO_NUM_35 // Remote IR source

// LCD backlight control
#define PIN_LCD_BACKLIGHT GPIO_NUM_26 // the gpio to be used in custom.c

// touch screen  T_DO is MISO, T_DIN is MOSI, T_CLK is CLk of the spi bus
#define PIN_TOUCH_CS GPIO_NUM_0 //Chip select T_CS

// init a gpio as output
void gpio_output_conf(gpio_num_t gpio);

// get the hardware partition infos
esp_err_t open_partition(const char *partition_label, const char *namespace, nvs_open_mode open_mode, nvs_handle *handle);
void close_partition(nvs_handle handle, const char *partition_label);
void gpio_get_label(char **label);
void gpio_get_comment(char **label);
void gpio_get_spi_bus(uint8_t *spi_no, gpio_num_t *miso, gpio_num_t *mosi, gpio_num_t *sclk);
void gpio_get_vs1053(gpio_num_t *xcs, gpio_num_t *xdcs, gpio_num_t *dreq);
void gpio_get_encoders(gpio_num_t *enca, gpio_num_t *encb, gpio_num_t *encbtn, gpio_num_t *enca1, gpio_num_t *encb1, gpio_num_t *encbtn1);
void gpio_get_i2c(gpio_num_t *sda, gpio_num_t *scl);
void gpio_get_spi_lcd(gpio_num_t *cs, gpio_num_t *a0, gpio_num_t *rstlcd);
void gpio_get_ir_signal(gpio_num_t *ir);
void gpio_get_lcd_backlightl(gpio_num_t *lcdb);
void gpio_get_tachometer(gpio_num_t *tach);
bool gpio_get_ir_key(nvs_handle handle, const char *key, uint32_t *out_value1, uint32_t *out_value2);
void gpio_get_touch(gpio_num_t *cs);
void gpio_get_ledgpio(gpio_num_t *enca);
void gpio_set_ledgpio(gpio_num_t enca);
void option_get_lcd_info(uint8_t *rt);
void option_set_lcd_info(uint8_t rt);
void option_get_ddmm(uint8_t *enca);
void option_set_ddmm(uint8_t enca);
void option_get_lcd_out(uint32_t *enca);
void option_set_lcd_out(uint32_t enca);
void gpio_get_ds18b20(gpio_num_t *ds18b20);

#endif
