/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#pragma once

#include "esp_system.h"
#include "tda7313.h"

#define APMODE 0
#define STA1 1
#define STA2 2
#define SSIDLEN 32
#define PASSLEN 64
#define HOSTLEN 24

#define _NTP0 "0.ru.pool.ntp.org"
#define _NTP1 "1.ru.pool.ntp.org"
#define _NTP2 "2.ru.pool.ntp.org"
#define _NTP3 "3.ru.pool.ntp.org"

#define TZONELEN 30
#define NTP_LEN 20

// константы для маски флагов опций
#define Y_DDMM 0x01 // Формат отображения даты на дисплее bit 0: 0 = MMDD, 1 = DDMM
#define N_DDMM 0xFE

#define Y_ROTAT 0x02 // Поворот изображения дисплея bit 1: 0 = нет  1 = поворот на 180 градусов
#define N_ROTAT 0xFD

#define Y_ENC 0x04 // bit 2: Half step of encoder
#define N_ENC 0xFB

#define Y_VOL 0x08 // Свободный бит
#define N_VOL 0xF7 // Свободный бит

#define Y_WIFIAUTO 0x10 // bit 4: wifi auto reconnect
#define N_WIFIAUTO 0xEF

#define Y_TOGGLETIME 0x20 // bit 5: TOGGLE time or main sreen
#define N_TOGGLETIME 0xDF

#define Y_PATCH 0x40 // bit 6: 0 patch load  1 no patch
#define N_PATCH 0xBF

#define Y_GPIOMODE 0x80 // bit 7: 0 = gpio default 1 = gpio NVS
#define N_GPIOMODE 0x7F

typedef enum backlight_mode_t
{
	NOT_ADJUSTABLE, //Нерегулируемая подсветка
	BY_TIME,		//По времени
	BY_LIGHTING,	//По уровню освещённости в помещении
	BY_HAND			//Ручная регулировка
} backlight_mode_t;

typedef enum ir_mode_t
{
	IR_DEFAULD, // Опрос кодов по-умолчанию
	IR_CUSTOM,	//Опрос пользовательских кодов
} ir_mode_t;

struct device_settings
{
	uint16_t cleared; // 0xAABB if initialized
	uint8_t dhcpEn1;
	uint8_t ipAddr1[4];
	uint8_t mask1[4];
	uint8_t gate1[4];
	uint8_t dhcpEn2;
	uint8_t ipAddr2[4];
	uint8_t mask2[4];
	uint8_t gate2[4];
	char ssid1[SSIDLEN];
	char ssid2[SSIDLEN];
	char pass1[PASSLEN];
	char pass2[PASSLEN];
	uint8_t current_ap; // 0 = AP mode, else STA mode: 1 = ssid1, 2 = ssid2
	uint8_t vol;
	int8_t treble;
	uint8_t bass;
	int8_t freqtreble;
	uint8_t freqbass;
	uint8_t spacial;
	uint16_t currentstation; //
	uint8_t autostart;		 // 0: stopped, 1: playing
	uint8_t i2sspeed;		 // 0 = 48kHz, 1 = 96kHz, 2 = 128kHz
	uint32_t uartspeed;		 // serial baud
	char bt_ver[15];		 // for version BT module
	char ua[39];			 // user agent
	uint8_t ntp_mode;		 // ntp_mode
	uint32_t sleepValue;
	uint32_t wakeValue;
	// esp32
	input_mode_t audio_input_num; //
	ir_mode_t ir_mode;			  // Режим работы ИК-пульта
	uint8_t trace_level;
	uint32_t lcd_out; // timeout in seconds to switch off the lcd. 0 = no timeout
	uint8_t options;  // Переменная для хранения состояния флагов различных опций
	char hostname[HOSTLEN];
	uint32_t tp_calx;
	uint32_t tp_caly;
	backlight_mode_t backlight_mode; //режим управления подсветкой
	uint8_t backlight_level,		 //общий уровень подсветки
		day_brightness,				 //уровень дневной подсвеки
		night_brightness,			 //уровень ночной подсветки
		begin_h,					 //Время ночной подсветки начало, часы
		begin_m,					 //Время ночной подсветки начало, минуты
		end_h,						 //Время ночной подсветки конец, часы
		end_m,						 //Время ночной подсветки конец, минуты
		FanControl,					 //Режим управления оборотами вентилятора
		min_temp,					 //Минимальная температура срабатывания автоматики управления оборотами
		max_temp,					 //Максимальная температура срабатывания выключения звука
		min_pwm,					 //Минимальный уровень оборотов вентилятора при включении питания
		hand_pwm;					 //Уровень оборотов при ручной регулировке

	char tzone[TZONELEN];
	char ntp_server[4][NTP_LEN];

} Device_Settings;

struct shoutcast_info
{
	char domain[73]; //url
	char file[116];	 //path
	char name[64];
	uint16_t port; //port
};

extern struct device_settings *g_device;

void partitions_init(void);
void copyDeviceSettings();
void restoreDeviceSettings();
bool eeSetData(int address, void *buffer, int size);
bool eeSetData1(int address, void *buffer, int size);
void eeErasesettings(void);
void eeEraseAll();
void saveStation(struct shoutcast_info *station, uint16_t position);
void saveMultiStation(struct shoutcast_info *station, uint16_t position, uint8_t number);
void eeEraseStations(void);
struct shoutcast_info *getStation(uint8_t position);
void saveDeviceSettings(struct device_settings *settings);
void saveDeviceSettingsVolume(struct device_settings *settings);
struct device_settings *getDeviceSettings();
struct device_settings *getDeviceSettingsSilent();
