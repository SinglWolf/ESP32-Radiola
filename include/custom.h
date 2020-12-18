/******************************************************************************
 * 
 * Copyright 2018 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/
#ifndef __custom_h__
#define __custom_h__
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

typedef enum LedChannel
{
	DISPLAY, // Дисплей
	FAN,	 //Вентилятор
	BUZZER,	 //Пищалка
} led_channel_e;

void LedBacklightInit();
void SetLedBacklight();
void ds18b20Task(void *pvParameters);
void tach_init();
void init_ds18b20();
void TachTimer(TimerHandle_t xTimer);
float getTemperature();
uint16_t getRpmFan();
void InitPWM();
void beep(uint8_t time);

#endif