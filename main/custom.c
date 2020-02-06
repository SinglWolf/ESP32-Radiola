/******************************************************************************
 * 
 * Copyright 2018 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
 * This file contain empty functions definition on the standard delivery
*******************************************************************************/
#include "custom.h"
#include "main.h"

#include <stdio.h>
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "gpios.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"
#include "eeprom.h"

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
gpio_num_t led;

gpio_num_t fanspeed;
gpio_num_t buzzer;

typedef enum led_channel_t {
	DISPLAY, // Дисплей
	FAN,	 //Вентилятор
	BUZZER,  //Пищалка
} led_channel_t;

led_channel_t channels;
ledc_channel_config_t ledc_channel_display, ledc_channel_fan, ledc_channel_buzzer;
ledc_timer_config_t ledc_timer_display, ledc_timer_fan, ledc_timer_buzzer;

// LedBacklight control a gpio to switch on or off the lcd backlight
// in addition of the sys.lcdout("x")  configuration for system with battries.

void LedBacklightInit()
{
	if (led != GPIO_NONE)
	{
		if (g_device->backlight_mode == NOT_ADJUSTABLE)
		{
			// gpio_output_conf(led);
			// gpio_set_level(led, 1);
			ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, DISPLAY, 255));
			ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, DISPLAY));
		}
		else
		{
			/* AdjustBrightInit(); */
		}
	}
}

void AdjustBrightInit()
{
	//Подготовка и настройка конфигурации таймера LED-контроллера
	ledc_timer_display.duty_resolution = LEDC_TIMER_8_BIT; // разрешение ШИМ
	ledc_timer_display.freq_hz = 5000;					   // частота сигнала ШИМ
	ledc_timer_display.speed_mode = LEDC_LOW_SPEED_MODE;   // режим таймера
	ledc_timer_display.timer_num = DISPLAY;				   // номер таймера
	// Установить конфигурацию таймера DISPLAY для низкоскоростного канала
	ledc_timer_config(&ledc_timer_display);

	ledc_channel_display.channel = DISPLAY;
	ledc_channel_display.duty = 0;
	ledc_channel_display.gpio_num = led;
	ledc_channel_display.speed_mode = LEDC_LOW_SPEED_MODE;
	ledc_channel_display.hpoint = 0;
	ledc_channel_display.timer_sel = DISPLAY;

	// Установить LED-контроллер с предварительно подготовленной конфигурацией
	ledc_channel_config(&ledc_channel_display);

	// Инициализацая службы fade
	//ledc_fade_func_install(0);
}

void FanPwmInit()
{
	//Подготовка и настройка конфигурации таймера LED-контроллера
	ledc_timer_fan.duty_resolution = LEDC_TIMER_8_BIT; // разрешение ШИМ
	ledc_timer_fan.freq_hz = 25000;					   // частота сигнала ШИМ
	ledc_timer_fan.speed_mode = LEDC_LOW_SPEED_MODE;   // режим таймера
	ledc_timer_fan.timer_num = FAN;					   // номер таймера
	// Установить конфигурацию таймера FAN для низкоскоростного канала
	ledc_timer_config(&ledc_timer_fan);

	ledc_channel_fan.channel = FAN;
	ledc_channel_fan.duty = 0;
	ledc_channel_fan.gpio_num = fanspeed;
	ledc_channel_fan.speed_mode = LEDC_LOW_SPEED_MODE;
	ledc_channel_fan.hpoint = 0;
	ledc_channel_fan.timer_sel = FAN;

	// Установить LED-контроллер с предварительно подготовленной конфигурацией
	ledc_channel_config(&ledc_channel_fan);

	// Инициализацая службы fade
	//ledc_fade_func_install(0);
}

void BuzzerInit()
{
	//Подготовка и настройка конфигурации таймера LED-контроллера
	ledc_timer_buzzer.speed_mode = LEDC_HIGH_SPEED_MODE;  // режим таймера
	ledc_timer_buzzer.duty_resolution = LEDC_TIMER_8_BIT; // разрешение ШИМ
	ledc_timer_buzzer.timer_num = BUZZER;				  // номер таймера
	ledc_timer_buzzer.freq_hz = 2000;					  // частота сигнала ШИМ
	// Установить конфигурацию таймера BUZZER для низкоскоростного канала
	ledc_timer_config(&ledc_timer_buzzer);

	ledc_channel_buzzer.gpio_num = buzzer;
	ledc_channel_buzzer.speed_mode = LEDC_HIGH_SPEED_MODE;
	ledc_channel_buzzer.channel = BUZZER;
	ledc_channel_buzzer.intr_type = LEDC_INTR_DISABLE;
	ledc_channel_buzzer.timer_sel = BUZZER;
	ledc_channel_buzzer.duty = 0x0; // 50%=0x3FFF, 100%=0x7FFF for 15 Bit
									// 50%=0x01FF, 100%=0x03FF for 10 Bit
									// 50%=0x7F,   100%=0xFF   for 8  Bit

	// Установить LED-контроллер с предварительно подготовленной конфигурацией
	ledc_channel_config(&ledc_channel_buzzer);

	// Инициализацая службы fade
	//ledc_fade_func_install(0);
}
void InitPWM()
{
	gpio_get_backlightl(&led, g_device->gpio_mode);
	if (led != GPIO_NONE)
		AdjustBrightInit();
	gpio_get_fanspeed(&fanspeed, g_device->gpio_mode);
	if (fanspeed != GPIO_NONE)
		FanPwmInit();
	gpio_get_buzzer(&buzzer, g_device->gpio_mode);
	if (buzzer != GPIO_NONE)
		BuzzerInit();
}

void beep(uint8_t time)
{
	if (buzzer != GPIO_NONE)
	{
		// start
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER, 0x7F); //
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER);
		vTaskDelay(time / portTICK_PERIOD_MS);
		// stop
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER, 0);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER);
	}
}

void LedBacklightOn()
{
	if (led != GPIO_NONE)
	{
		//gpio_set_level(led, 1);
		ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, DISPLAY, 255));
		ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, DISPLAY));
	}
}

void LedBacklightOff()
{
	if (led != GPIO_NONE)
	{ //gpio_set_level(led, 0);
		ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, DISPLAY, 0));
		ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, DISPLAY));
	}
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
	// Create DS18B20 devices on the 1-Wire bus
	DS18B20_Info *ds18b20_info = ds18b20_malloc(); // heap allocation

	ESP_LOGI(TAG, "Single device optimisations enabled\n");
	ds18b20_init_solo(ds18b20_info, owb); // only one device on bus

	ds18b20_use_crc(ds18b20_info, true); // enable CRC check for temperature readings
	ds18b20_set_resolution(ds18b20_info, _RESOLUTION);
	DS18B20_ERROR error = status;
	while (error == DS18B20_OK)
	{
		ds18b20_convert_all(owb);

		// In this application all devices use the same resolution,
		// so use the first device to determine the delay
		ds18b20_wait_for_conversion(ds18b20_info);

		// Read the results immediately after conversion otherwise it may fail
		// (using printf before reading may take too long)

		error = ds18b20_read_temp(ds18b20_info, &cur_temp);

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
}
/* Initialize tachometer */
void tach_init()
{
	if (gpio_get_tachometer(&tach, g_device->gpio_mode) == ESP_OK)
	{
		if (tach != GPIO_NONE)
		{
			tachTmr = xTimerCreate("TachTimer", pdMS_TO_TICKS(interval), pdTRUE, (void *)id, &TachTimer);
			if (xTimerStart(tachTmr, 10) != pdPASS)
			{
				ESP_LOGE(TAG, "TachTimer starting fail");
				return;
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
			ESP_LOGE(TAG, "Tachometer not present.");
	}
	else
		ESP_LOGE(TAG, "Tachometer init fail!.");
	ESP_LOGD(TAG, "Tachometer init OK.");
}
void TachTimer(TimerHandle_t xTimer)
{
	if (pcnt_get_counter_value(PCNT_TACHOME, &rpm_fan) == ESP_OK)
	{ //ESP_LOGI(TAG, "Current counter value :%d\n", rpm_fan * 60);
		pcnt_counter_pause(PCNT_TACHOME);
		pcnt_counter_clear(PCNT_TACHOME);
		pcnt_counter_resume(PCNT_TACHOME);
	}
	else
	{
		ESP_LOGE(TAG, "Tachometer value not read!");
	}
}
float getTemperature()
{
	return cur_temp;
}
uint16_t getRpmFan()
{
	return (uint16_t)rpm_fan;
}
void init_ds18b20()
{
	if (gpio_get_ds18b20(&ds18b20, g_device->gpio_mode) == ESP_OK)
	{
		if (ds18b20 != GPIO_NONE)
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
				owb_uninitialize(owb);
			}
			else
			{
				ESP_LOGE(TAG, "An error occurred reading ROM code: %d", status);
				owb_uninitialize(owb);
				return;
			}

			xTaskHandle pxCreatedTask;
			xTaskCreatePinnedToCore(ds18b20Task, "ds18b20Task", 2200, NULL, PRIO_DS18B20, &pxCreatedTask, CPU_DS18B20);
			vTaskDelay(1);
			ESP_LOGI(TAG, "%s task: %x", "ds18b20Task", (unsigned int)pxCreatedTask);
			return;
		}
		else
			ESP_LOGE(TAG, "ds18b20 not present.");
	}
	else
		ESP_LOGE(TAG, "ds18b20 init fail!");
}
