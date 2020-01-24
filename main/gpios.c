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
const char ircodes_space[] = {"ircodes_space"};
const char gpio_space[] = {"gpio_space"};

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

esp_err_t gpio_set_spi_bus(uint8_t spi_no, gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_spi_bus");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "K_SPI", spi_no);
	err |= nvs_set_u8(hardware_handle, "P_MISO", (uint8_t)miso);
	err |= nvs_set_u8(hardware_handle, "P_MOSI", (uint8_t)mosi);
	err |= nvs_set_u8(hardware_handle, "P_CLK", (uint8_t)sclk);
	if (err != ESP_OK)
		ESP_LOGE(TAG, "gpio_set_spi_bus err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_spi_bus(uint8_t *spi_no, gpio_num_t *miso, gpio_num_t *mosi, gpio_num_t *sclk, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		if (miso != NULL)
			*miso = PIN_NUM_MISO;
		if (mosi != NULL)
			*mosi = PIN_NUM_MOSI;
		if (sclk != NULL)
			*sclk = PIN_NUM_CLK;
		if (spi_no != NULL)
			*spi_no = KSPI;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "in spi_bus err 0x%x", err);
		return err;
	}

	uint8_t val;
	if (spi_no != NULL)
	{
		err = nvs_get_u8(hardware_handle, "K_SPI", &val);
		*spi_no = val;
	}
	if (miso != NULL)
	{
		err |= nvs_get_u8(hardware_handle, "P_MISO", &val);
		*miso = val;
	}
	if (mosi != NULL)
	{
		err |= nvs_get_u8(hardware_handle, "P_MOSI", &val);
		*mosi = val;
	}
	if (sclk != NULL)
	{
		err |= nvs_get_u8(hardware_handle, "P_CLK", &val);
		*sclk = val;
	}
	if (err != ESP_OK)
		ESP_LOGE(TAG, "g_get_spi_bus err 0x%x", err);
	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_vs1053(gpio_num_t *xcs, gpio_num_t *xdcs, gpio_num_t *dreq, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*xcs = PIN_NUM_XCS;
		*xdcs = PIN_NUM_XDCS;
		*dreq = PIN_NUM_DREQ;
		return 0;
	}
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_get_vs1053 err 0x%x", err);
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_XCS", &val);
	*xcs = val;
	err |= nvs_get_u8(hardware_handle, "P_XDCS", &val);
	*xdcs = val;
	err |= nvs_get_u8(hardware_handle, "P_DREQ", &val);
	*dreq = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_get_vs1053 err 0x%x", err);
	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_vs1053(gpio_num_t xcs, gpio_num_t xdcs, gpio_num_t dreq)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_vs1053");
		return err;
	}
	err = nvs_set_u8(hardware_handle, "P_XCS", (uint8_t)xcs);
	err |= nvs_set_u8(hardware_handle, "P_XDCS", (uint8_t)xdcs);
	err |= nvs_set_u8(hardware_handle, "P_DREQ", (uint8_t)dreq);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_vs1053 err 0x%x", err);
	close_partition(hardware_handle, hardware);
	return err;
}

void option_get_lcd_rotat(uint8_t *rt)
{
	*rt = ((g_device->options32) & T_ROTAT) ? 1 : 0;
}

void option_set_lcd_rotat(uint8_t rt)
{
	if (rt == 0)
		g_device->options32 &= NT_ROTAT;
	else
		g_device->options32 |= T_ROTAT;
	saveDeviceSettings(g_device);
}

void option_get_ddmm(uint8_t *ddmm)
{
	*ddmm = ((g_device->options32) & T_DDMM) ? 1 : 0;
}

void option_set_ddmm(uint8_t ddmm)
{
	if (ddmm == 0)
		g_device->options32 &= NT_DDMM;
	else
		g_device->options32 |= T_DDMM;
	saveDeviceSettings(g_device);
}

void option_get_lcd_out(uint32_t *lcd_out)
{
	*lcd_out = g_device->lcd_out;
}
void option_set_lcd_out(uint32_t lcd_out)
{
	g_device->lcd_out = lcd_out;
	saveDeviceSettings(g_device);
}

esp_err_t gpio_get_ledgpio(gpio_num_t *ledgpio, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*ledgpio = GPIO_LED;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in ledgpio");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_LED_GPIO", &val);
	*ledgpio = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_ledgpio err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_ledgpio(gpio_num_t ledgpio)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in set_ledgpio");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_LED_GPIO", ledgpio);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ledgpio err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_encoders(gpio_num_t *enca, gpio_num_t *encb, gpio_num_t *encbtn, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*enca = PIN_ENC0_A;
		*encb = PIN_ENC0_B;
		*encbtn = PIN_ENC0_BTN;
		return 0;
	}
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in enc");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_ENC0_A", &val);
	*enca = val;
	err |= nvs_get_u8(hardware_handle, "P_ENC0_B", &val);
	*encb = val;
	err |= nvs_get_u8(hardware_handle, "P_ENC0_BTN", &val);
	*encbtn = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_encoder0 err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_encoders(gpio_num_t enca, gpio_num_t encb, gpio_num_t encbtn)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_encoders");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_ENC0_A", (uint8_t)enca);
	err |= nvs_set_u8(hardware_handle, "P_ENC0_B", (uint8_t)encb);
	err |= nvs_set_u8(hardware_handle, "P_ENC0_BTN", (uint8_t)encbtn);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_encoders err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_i2c(gpio_num_t *sda, gpio_num_t *scl, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*scl = PIN_I2C_SCL;
		*sda = PIN_I2C_SDA;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in i2c");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_I2C_SCL", &val);
	*scl = val;
	err |= nvs_get_u8(hardware_handle, "P_I2C_SDA", &val);
	*sda = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_i2c err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_i2c(gpio_num_t sda, gpio_num_t scl)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_i2c");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_I2C_SCL", (uint8_t)scl);
	err |= nvs_set_u8(hardware_handle, "P_I2C_SDA", (uint8_t)sda);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_i2c err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_spi_lcd(gpio_num_t *cs, gpio_num_t *a0, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*cs = PIN_LCD_CS;
		*a0 = PIN_LCD_A0;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in spi_lcd");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_LCD_CS", &val);
	*cs = val;
	err |= nvs_get_u8(hardware_handle, "P_LCD_A0", &val);
	*a0 = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_spi_lcd err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_spi_lcd(gpio_num_t cs, gpio_num_t a0)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_spi_lcd");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_LCD_CS", (uint8_t)cs);
	err |= nvs_set_u8(hardware_handle, "P_LCD_A0", (uint8_t)a0);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_spi_lcd err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_ir_signal(gpio_num_t *ir, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*ir = PIN_IR_SIGNAL;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in ir");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_IR_SIGNAL", &val);
	*ir = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_ir_signal err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_ir_signal(gpio_num_t ir)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_ir_signal");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_IR_SIGNAL", (uint8_t)ir);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ir_signal err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_lcd_backlightl(gpio_num_t *lcdb, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*lcdb = PIN_LCD_BACKLIGHT;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in lcdback");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_BACKLIGHT", &val);
	*lcdb = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_lcd_backlightl err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_lcd_backlightl(gpio_num_t lcdb)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_lcd_backlightl");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_BACKLIGHT", (uint8_t)lcdb);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_lcd_backlightl err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_tachometer(gpio_num_t *tach, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*tach = PIN_NUM_TACH;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in tachometer");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_TACHOMETER", &val);
	*tach = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_tachometer err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_tachometer(gpio_num_t tach)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_tachometer");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_TACHOMETER", (uint8_t)tach);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_tachometer err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_ds18b20(gpio_num_t *ds18b20, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*ds18b20 = PIN_NUM_DS18B20;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in ds18b20");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_DS18B20", &val);
	*ds18b20 = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_ds18b20 err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_ds18b20(gpio_num_t ds18b20)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_ds18b20");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_DS18B20", (uint8_t)ds18b20);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ds18b20 err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_touch(gpio_num_t *touch, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*touch = PIN_TOUCH_CS;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in touch");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_TOUCH_CS", &val);
	*touch = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_touch err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_touch(gpio_num_t touch)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_touch");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_TOUCH_CS", (uint8_t)touch);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_touch err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_fanspeed(gpio_num_t *fanspeed, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*fanspeed = PIN_NUM_PWM;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in fanspeed");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_FAN_SPEED", &val);
	*fanspeed = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_fanspeed err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_fanspeed(gpio_num_t fanspeed)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_fanspeed");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_FAN_SPEED", (uint8_t)fanspeed);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_fanspeed err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_buzzer(gpio_num_t *buzzer, bool gpio_mode)
{
	// init default
	if (!gpio_mode)
	{
		*buzzer = PIN_NUM_BUZZ;
		return 0;
	}

	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in buzzer");
		return err;
	}

	uint8_t val;
	err = nvs_get_u8(hardware_handle, "P_BUZZER", &val);
	*buzzer = val;
	if (err != ESP_OK)
		ESP_LOGD(TAG, "g_get_buzzer err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_buzzer(gpio_num_t buzzer)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_buzzer");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_BUZZER", (uint8_t)buzzer);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_buzzer err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
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