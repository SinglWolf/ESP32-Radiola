/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#pragma once

#include "esp_system.h"
#include "tda7313.h"
//define for bit array options
#define T_THEME 1
#define NT_THEME 0xFE
#define T_PATCH 2
#define NT_PATCH 0xFD
//define for bit array options32
#define T_DDMM 1
#define NT_DDMM 0xFE
#define T_ROTAT 2
#define NT_ROTAT 0xFD
#define T_ENC0 4
#define T_ENC1 8
#define NT_ENC0 0xFB
#define NT_ENC1 0xF7
#define T_WIFIAUTO 0x10
#define NT_WIFIAUTO 0xEF
#define T_TOGGLETIME 0x20
#define NT_TOGGLETIME 0xDF

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

typedef enum backlight_mode_t {
	NOT_ADJUSTABLE, //Нерегулируемая подсветка
	BY_TIME,		//По времени
	BY_LIGHTING,	//По уровню освещённости в помещении
	BY_HAND			//Ручная регулировка
} backlight_mode_t;

struct device_settings
{
	uint16_t cleared; // 0xAABB if initialized
	uint8_t gpio_mode;
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
	uint8_t options;		 // bit0:0 theme ligth blue, 1 Dark brown, bit1: 0 patch load  1 no patch
	char ua[39];			 // user agent
	uint8_t ntp_mode;		 // ntp_mode
	uint32_t sleepValue;
	uint32_t wakeValue;
	// esp32
	input_mode_t audio_input_num; //
	uint8_t trace_level;
	uint32_t lcd_out;  // timeout in seconds to switch off the lcd. 0 = no timeout
	uint8_t options32; // bit0:0 = MMDD, 1 = DDMM  in the time display, bit1: 0= lcd without rotation  1 = lcd rotated 180
					   // bit 2: Half step of encoder0, bit3: Half step of encoder1, bit4: wifi auto reconnect
					   // bit5: TOGGLE time or main sreen
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
	char file[116];  //path
	char name[64];
	int8_t ovol;   // offset volume
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
