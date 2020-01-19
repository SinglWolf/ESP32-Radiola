/******************************************************************************
 * 
 * Copyright 2018 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
 * This file contain empty functions definition on the standard delivery
*******************************************************************************/
#include "custom.h"
#include "main.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "gpios.h"

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#define TAG "custom"

#define PCNT_TACHOME PCNT_UNIT_0

#define _RESOLUTION DS18B20_RESOLUTION_12_BIT

TimerHandle_t tachTmr;
int id = 1;
int interval = 1000;
int16_t rpm_fan = 0;
float cur_temp = 0;

gpio_num_t tach;
gpio_num_t ds18b20;
gpio_num_t lcdb;
// LedBacklight control a gpio to switch on or off the lcd backlight
// in addition of the sys.lcdout("x")  configuration for system with battries.

void LedBacklightInit()
{
	gpio_get_lcd_backlightl(&lcdb);
	if (lcdb != GPIO_NONE)
	{
		gpio_output_conf(lcdb);
		gpio_set_level(lcdb, 1);
	}
}

void LedBacklightOn()
{
	if (lcdb != GPIO_NONE)
		gpio_set_level(lcdb, 1);
}

void LedBacklightOff()
{
	if (lcdb != GPIO_NONE)
		gpio_set_level(lcdb, 0);
}

void ds18b20Task(void *pvParameters)
{
	// Create a 1-Wire bus, using the RMT timeslot driver
	OneWireBus *owb;
	owb_rmt_driver_info rmt_driver_info;
	owb = owb_rmt_initialize(&rmt_driver_info, ds18b20, RMT_CHANNEL_2, RMT_CHANNEL_1);
	owb_use_crc(owb, true); // enable CRC check for ROM code

	// For a single device only:
	OneWireBus_ROMCode rom_code;
	owb_status status = owb_read_rom(owb, &rom_code);
	if (status == OWB_STATUS_OK)
	{
		char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
		owb_string_from_rom_code(rom_code, rom_code_s, sizeof(rom_code_s));
		ESP_LOGI(TAG, "Single device %s present\n", rom_code_s);
	}
	else
	{
		ESP_LOGE(TAG, "An error occurred reading ROM code: %d", status);
	}

	// Create DS18B20 devices on the 1-Wire bus
	DS18B20_Info *ds18b20_info = ds18b20_malloc(); // heap allocation

	ESP_LOGI(TAG, "Single device optimisations enabled\n");
	ds18b20_init_solo(ds18b20_info, owb); // only one device on bus

	ds18b20_use_crc(ds18b20_info, true); // enable CRC check for temperature readings
	ds18b20_set_resolution(ds18b20_info, _RESOLUTION);

	// Read temperatures more efficiently by starting conversions on all devices at the same time
	while (1)
	{
		ds18b20_convert_all(owb);

		// In this application all devices use the same resolution,
		// so use the first device to determine the delay
		ds18b20_wait_for_conversion(ds18b20_info);

		// Read the results immediately after conversion otherwise it may fail
		// (using printf before reading may take too long)

		DS18B20_ERROR error = ds18b20_read_temp(ds18b20_info, &cur_temp);

		if (error != DS18B20_OK)
		{
			ESP_LOGE(TAG, "DS18B20 ERROR: %d\n", error);
		}

		//int uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
		//ESP_LOGD("ds18b20Task", striWATERMARK, uxHighWaterMark, xPortGetFreeHeapSize());

		vTaskDelay(10);
	}

	// clean up dynamically allocated data

	ds18b20_free(&ds18b20_info);

	owb_uninitialize(owb);

	ESP_LOGI(TAG, "Restarting now.\n");
	fflush(stdout);
	vTaskDelay(100);
	esp_restart();
}
/* Initialize tachometer */
void tach_init()
{
	gpio_get_tachometer(&tach);
	if (tach != GPIO_NONE)
	{
		tachTmr = xTimerCreate("TachTimer", pdMS_TO_TICKS(interval), pdTRUE, (void *)id, &TachTimer);
		if (xTimerStart(tachTmr, 10) != pdPASS)
		{
			printf("TachTimer start error");
		}
		/* Prepare configuration for the PCNT unit */
		pcnt_config_t pcnt_config = {
			// Set PCNT input signal and control GPIOs
			.pulse_gpio_num = tach,
			.ctrl_gpio_num = PCNT_PIN_NOT_USED,
			.channel = PCNT_CHANNEL_0,
			.unit = PCNT_TACHOME,
			// What to do on the positive / negative edge of pulse input?
			.pos_mode = PCNT_COUNT_INC, // Count up on the positive edge
			.neg_mode = PCNT_COUNT_DIS, // Keep the counter value on the negative edge
			// What to do when control input is low or high?
			.lctrl_mode = PCNT_MODE_KEEP,	// Reverse counting direction if low
			.hctrl_mode = PCNT_MODE_REVERSE, // Keep the primary counter mode if high
			// Set the maximum and minimum limit values to watch
			.counter_h_lim = 6000,
			.counter_l_lim = 0,
		};
		/* Initialize PCNT unit */
		pcnt_unit_config(&pcnt_config);
		/* Configure and enable the input filter */
		pcnt_set_filter_value(PCNT_TACHOME, 50);
		pcnt_filter_enable(PCNT_TACHOME);

		/* Initialize PCNT's counter */
		pcnt_counter_pause(PCNT_TACHOME);
		pcnt_counter_clear(PCNT_TACHOME);

		/* Everything is set up, now go to counting */
		pcnt_counter_resume(PCNT_TACHOME);
	}
	else
	{
		ESP_LOGI(TAG, "Tachometer not present.");
	}
}
void TachTimer(TimerHandle_t xTimer)
{
	pcnt_get_counter_value(PCNT_TACHOME, &rpm_fan);
	//ESP_LOGI(TAG, "Current counter value :%d\n", rpm_fan * 60);
	pcnt_counter_pause(PCNT_TACHOME);
	pcnt_counter_clear(PCNT_TACHOME);
	pcnt_counter_resume(PCNT_TACHOME);
}
float getTemperature()
{
	return cur_temp;
}
int16_t getRpmFan()
{
	return rpm_fan;
}
void init_ds18b20()
{
	gpio_get_ds18b20(&ds18b20);
	if (ds18b20 != GPIO_NONE)
	{
		xTaskHandle pxCreatedTask;
		xTaskCreatePinnedToCore(ds18b20Task, "ds18b20Task", 1800, NULL, PRIO_DS18B20, &pxCreatedTask, CPU_DS18B20);
		vTaskDelay(1);
		ESP_LOGI(TAG, "%s task: %x", "ds18b20Task", (unsigned int)pxCreatedTask);
	}
}
