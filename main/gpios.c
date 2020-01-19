/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define TAG "GPIO"
#include "driver/gpio.h"
#include "gpios.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "main.h"
#include "eeprom.h"

static xSemaphoreHandle muxnvs = NULL;
const char hardware[] = {"hardware"};
const char option_space[] = {"option_space"};
const char gpio_space[] = {"gpio_space"};
const char label_space[] = {"label_space"};

// init a gpio as output
void gpio_output_conf(gpio_num_t gpio)
{
	gpio_config_t gpio_conf;
	gpio_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1) << gpio));
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&gpio_conf);
	gpio_set_level(gpio, 1);
}

// open and read the gpio hardware setting
//
esp_err_t open_partition(const char *partition_label, const char *namespace, nvs_open_mode open_mode, nvs_handle *handle)
{
	esp_err_t err;
	if (muxnvs == NULL)
		muxnvs = xSemaphoreCreateMutex();
	xSemaphoreTake(muxnvs, portMAX_DELAY);
	err = nvs_flash_init_partition(partition_label);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "Hardware partition not found");
		return err;
	}
	//	ESP_ERROR_CHECK(nvs_open_from_partition(partition_label, namespace, open_mode, handle));
	err = nvs_open_from_partition(partition_label, namespace, open_mode, handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "Namespace %s not found, ERR: %x", namespace, err);
		nvs_flash_deinit_partition(partition_label);
		xSemaphoreGive(muxnvs);
	}
	return err;
}
void close_partition(nvs_handle handle, const char *partition_label)
{
	nvs_commit(handle); // if a write is pending
	nvs_close(handle);
	nvs_flash_deinit_partition(partition_label);
	xSemaphoreGive(muxnvs);
}

void gpio_get_label(char **label)
{
	size_t required_size;
	nvs_handle hardware_handle;
	*label = NULL;
	if (open_partition(hardware, label_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in get label");
		return;
	}
	nvs_get_str(hardware_handle, "L_LABEL", NULL, &required_size);
	if (required_size > 1)
	{
		*label = malloc(required_size);
		nvs_get_str(hardware_handle, "L_LABEL", *label, &required_size);
		ESP_LOGV(TAG, "Label: \"%s\"\n Required size: %d", *label, required_size);
	}
	close_partition(hardware_handle, hardware);
}

void gpio_get_comment(char **label)
{
	size_t required_size;
	nvs_handle hardware_handle;
	*label = NULL;
	if (open_partition(hardware, label_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in get comment");
		return;
	}
	nvs_get_str(hardware_handle, "L_COMMENT", NULL, &required_size);
	if (required_size > 1)
	{
		*label = malloc(required_size);
		nvs_get_str(hardware_handle, "L_COMMENT", *label, &required_size);
		ESP_LOGV(TAG, "Label: \"%s\"\n Required size: %d", *label, required_size);
	}
	close_partition(hardware_handle, hardware);
}

void gpio_get_spi_bus(uint8_t *spi_no, gpio_num_t *miso, gpio_num_t *mosi, gpio_num_t *sclk)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	if (miso != NULL)
		*miso = PIN_NUM_MISO;
	if (mosi != NULL)
		*mosi = PIN_NUM_MOSI;
	if (sclk != NULL)
		*sclk = PIN_NUM_CLK;
	if (spi_no != NULL)
		*spi_no = KSPI;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in spi_bus");
		return;
	}
	err = ESP_OK;
	if (spi_no != NULL)
		err = nvs_get_u8(hardware_handle, "K_SPI", (uint8_t *)spi_no);
	if (miso != NULL)
		err |= nvs_get_u8(hardware_handle, "P_MISO", (uint8_t *)miso);
	if (mosi != NULL)
		err |= nvs_get_u8(hardware_handle, "P_MOSI", (uint8_t *)mosi);
	if (sclk != NULL)
		err |= nvs_get_u8(hardware_handle, "P_CLK", (uint8_t *)sclk);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_spi_bus err 0x%x", err);
	close_partition(hardware_handle, hardware);
}

void gpio_get_vs1053(gpio_num_t *xcs, gpio_num_t *xdcs, gpio_num_t *dreq)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*xcs = PIN_NUM_XCS;
	*xdcs = PIN_NUM_XDCS;
	*dreq = PIN_NUM_DREQ;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in vs1053");
		return;
	}
	err = nvs_get_u8(hardware_handle, "P_XCS", (uint8_t *)xcs);
	err |= nvs_get_u8(hardware_handle, "P_XDCS", (uint8_t *)xdcs);
	err |= nvs_get_u8(hardware_handle, "P_DREQ", (uint8_t *)dreq);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_vs1053 err 0x%x", err);
	close_partition(hardware_handle, hardware);
}

void option_get_lcd_info(uint8_t *rt)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	uint8_t rot;
	// init default
	*rt = ((g_device->options32) & T_ROTAT) ? 1 : 0;
	if (open_partition(hardware, option_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in lcd_info");
		return;
	}

	err = nvs_get_u8(hardware_handle, "O_LCD_ROTA", (uint8_t *)&rot);
	if (rot != 255)
		*rt = rot;
	if (*rt)
		*rt = 1;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "oget_lcd_info err 0x%x", err);
	close_partition(hardware_handle, hardware);
}
void option_set_lcd_info(uint8_t rt)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space, NVS_READWRITE, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in set lcd_info");
		return;
	}

	err = nvs_set_u8(hardware_handle, "O_LCD_ROTA", rt ? 1 : 0);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "oset_lcd_info err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void option_get_ddmm(uint8_t *ddmm)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	uint8_t dmm;
	// init default
	*ddmm = ((g_device->options32) & T_DDMM) ? 1 : 0;
	;

	if (open_partition(hardware, option_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in ddmm");
		return;
	}

	err = nvs_get_u8(hardware_handle, "O_DDMM_FLAG", (uint8_t *)&dmm);

	if (err != ESP_OK)
		ESP_LOGD(TAG, "oget_ddmm err 0x%x", err);
	else
	{
		if (dmm != 255)
			*ddmm = dmm;
		if (*ddmm)
			*ddmm = 1;
	}

	close_partition(hardware_handle, hardware);
}
void option_set_ddmm(uint8_t ddmm)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space, NVS_READWRITE, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in set_ddmm");
		return;
	}

	err = nvs_set_u8(hardware_handle, "O_DDMM_FLAG", ddmm ? 1 : 0);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "oset_ddmm err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void option_get_lcd_out(uint32_t *lcd_out)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	uint32_t lout;
	// init default
	*lcd_out = g_device->lcd_out;

	if (open_partition(hardware, option_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in lcd_out");
		return;
	}

	err = nvs_get_u32(hardware_handle, "O_LCD_OUT", (uint32_t *)&lout);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "oget_lcd_out err 0x%x", err);
	else
	{
		if (lout == 255)
			lout = 0; // special case
		*lcd_out = lout;
	}
	close_partition(hardware_handle, hardware);
}
void option_set_lcd_out(uint32_t lcd_out)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space, NVS_READWRITE, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in set_lcd_out");
		return;
	}

	err = nvs_set_u32(hardware_handle, "O_LCD_OUT", lcd_out);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "oset_lcd_out err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_ledgpio(gpio_num_t *ledgpio)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*ledgpio = GPIO_LED;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in ledgpio");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_LED_GPIO", (uint8_t *)ledgpio);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_ledgpio err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_set_ledgpio(gpio_num_t ledgpio)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in set_ledgpio");
		return;
	}

	err = nvs_set_u8(hardware_handle, "P_LED_GPIO", ledgpio);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ledgpio err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_encoders(gpio_num_t *enca, gpio_num_t *encb, gpio_num_t *encbtn, gpio_num_t *enca1, gpio_num_t *encb1, gpio_num_t *encbtn1)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	if (bigSram()) // default is not compatible (gpio 16 & 17)
	{
		*enca = GPIO_NONE;
		*encb = GPIO_NONE;
		*encbtn = GPIO_NONE;
		*enca1 = GPIO_NONE;
		*encb1 = GPIO_NONE;
		*encbtn1 = GPIO_NONE;
	}
	else
	{
		*enca = PIN_ENC0_A;
		*encb = PIN_ENC0_B;
		*encbtn = PIN_ENC0_BTN;
		*enca1 = PIN_ENC1_A;
		*encb1 = PIN_ENC1_B;
		*encbtn1 = PIN_ENC1_BTN;
	}

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in enc");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_ENC0_A", (uint8_t *)enca);
	err |= nvs_get_u8(hardware_handle, "P_ENC0_B", (uint8_t *)encb);
	err |= nvs_get_u8(hardware_handle, "P_ENC0_BTN", (uint8_t *)encbtn);
	err |= nvs_get_u8(hardware_handle, "P_ENC1_A", (uint8_t *)enca1);
	err |= nvs_get_u8(hardware_handle, "P_ENC1_B", (uint8_t *)encb1);
	err |= nvs_get_u8(hardware_handle, "P_ENC1_BTN", (uint8_t *)encbtn1);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_encoder0 err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_i2c(gpio_num_t *sda, gpio_num_t *scl)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*scl = PIN_I2C_SCL;
	*sda = PIN_I2C_SDA;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in i2c");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_I2C_SCL", (uint8_t *)scl);
	err |= nvs_get_u8(hardware_handle, "P_I2C_SDA", (uint8_t *)sda);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_i2c err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_spi_lcd(gpio_num_t *cs, gpio_num_t *a0, gpio_num_t *rstlcd)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*cs = PIN_LCD_CS;
	*a0 = PIN_LCD_A0;
	*rstlcd = PIN_LCD_RST;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in spi_lcd");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_LCD_CS", (uint8_t *)cs);
	err |= nvs_get_u8(hardware_handle, "P_LCD_A0", (uint8_t *)a0);
	err |= nvs_get_u8(hardware_handle, "P_LCD_RST", (uint8_t *)rstlcd);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_spi_lcd err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_ir_signal(gpio_num_t *ir)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*ir = PIN_IR_SIGNAL;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in ir");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_IR_SIGNAL", (uint8_t *)ir);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_ir_signal err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_lcd_backlightl(gpio_num_t *lcdb)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*lcdb = PIN_LCD_BACKLIGHT;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in lcdback");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_BACKLIGHT", (uint8_t *)lcdb);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_lcd_backlightl err 0x%x", err);

	close_partition(hardware_handle, hardware);
}
void gpio_get_tachometer(gpio_num_t *tach)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*tach = PIN_NUM_TACH;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in tachometer");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_TACHOMETER", (uint8_t *)tach);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_tachometer err 0x%x", err);

	close_partition(hardware_handle, hardware);
}
void gpio_get_ds18b20(gpio_num_t *ds18b20)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*ds18b20 = PIN_NUM_DS18B20;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in ds18b20");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_DS18B20", (uint8_t *)ds18b20);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_ds18b20 err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_touch(gpio_num_t *touch)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*touch = PIN_TOUCH_CS;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in touch");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_TOUCH_CS", (uint8_t *)touch);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_touch err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

void gpio_get_fanspeed(gpio_num_t *fanspeed)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*fanspeed = PIN_NUM_PWM;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in fanspeed");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_FAN_SPEED", (uint8_t *)fanspeed);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_fanspeed err 0x%x", err);

	close_partition(hardware_handle, hardware);
}
void gpio_get_buzzer(gpio_num_t *buzzer)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*buzzer = PIN_NUM_PWM;

	if (open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle) != ESP_OK)
	{
		ESP_LOGD(TAG, "in buzzer");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_BUZZER", (uint8_t *)buzzer);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_buzzer err 0x%x", err);

	close_partition(hardware_handle, hardware);
}

bool gpio_get_ir_key(nvs_handle handle, const char *key, uint32_t *out_value1, uint32_t *out_value2)
{
	// init default
	bool ret = false;
	*out_value1 = 0;
	*out_value2 = 0;
	size_t required_size;
	nvs_get_str(handle, key, NULL, &required_size);
	if (required_size > 1)
	{
		char *string = malloc(required_size);
		nvs_get_str(handle, key, string, &required_size);
		sscanf(string, "%x %x", out_value1, out_value2);
		//		ESP_LOGV(TAG,"String \"%s\"\n Required size: %d",string,required_size);
		free(string);
		ret = true;
	}
	ESP_LOGV(TAG, "Key: %s, value1: %x, value2: %x, ret: %d", key, *out_value1, *out_value2, ret);

	return ret;
}