/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#pragma once

#include "esp_system.h"
#include "tda7313.h"
#include "gpios.h"

#define APMODE 0
#define STA1 1
#define STA2 2
#define SSIDLEN 32
#define PASSLEN 64
#define HOSTLEN 24

#define TZONELEN 30
#define NTP_LEN 20

#define MAIN_SPACE "radiola"
#define STATION_SPACE "stations"

#define MAXSTATIONS 128 // Максимальное количество станций

// константы для маски флагов опций
#define Y_DDMM 0x01 // bit 0: Формат отображения даты на дисплее 0 = MMDD, 1 = DDMM
#define N_DDMM 0xFE

#define Y_ROTAT 0x02 // bit 1: Поворот изображения дисплея 0 = нет  1 = поворот на 180 градусов
#define N_ROTAT 0xFD

#define Y_ENC 0x04 // bit 2: Half step of encoder
#define N_ENC 0xFB

#define Y_BACKLIGHT 0x08 // bit 3: Регулируемая подсветка дисплея 0 = нету 1 = есть
#define N_BACKLIGHT 0xF7 //

#define Y_WIFIAUTO 0x10 // bit 4: wifi auto reconnect
#define N_WIFIAUTO 0xEF

#define Y_TOGGLETIME 0x20 // bit 5: TOGGLE time or main sreen
#define N_TOGGLETIME 0xDF

#define Y_FAN 0x40 // bit 6: Регулировка оборотов вентилятора 0 = нету 1 = есть
#define N_FAN 0xBF

#define Y_GPIOMODE 0x80 // bit 7: 0 = gpio default 1 = gpio from NVS
#define N_GPIOMODE 0x7F

typedef enum backlight_mode
{
	NOT_ADJUSTABLE, //Нерегулируемая подсветка
	BY_TIME,		//По времени
	BY_LIGHTING,	//По уровню освещённости в помещении
	BY_HAND			//Ручная регулировка
} backlight_mode_e;

typedef enum ir_mode
{
	IR_DEFAULD, // Опрос кодов по-умолчанию
	IR_CUSTOM,	//Опрос пользовательских кодов
} ir_mode_e;

typedef struct radiola_config
{
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
	uint8_t CurrentStation; //
	uint8_t TotalStations;	 // Общее количество станций
	uint8_t autostart;		 // 0: stopped, 1: playing
	uint8_t i2sspeed;		 // 0 = 48kHz, 1 = 96kHz, 2 = 128kHz
	uint32_t uartspeed;		 // serial baud
	char bt_ver[15];		 // for version BT module
	char ua[39];			 // user agent
	uint8_t ntp_mode;		 // ntp_mode
	uint32_t sleepValue;
	uint32_t wakeValue;
	input_mode_e audio_input_num; //
	ir_mode_e ir_mode;			  // Режим работы ИК-пульта
	uint8_t trace_level;
	uint32_t lcd_out; // timeout in seconds to switch off the lcd. 0 = no timeout
	uint8_t options;  // Переменная для хранения состояния флагов различных опций
	char hostname[HOSTLEN];
	uint32_t tp_calx;
	uint32_t tp_caly;
	backlight_mode_e backlight_mode; // режим управления подсветкой
	uint8_t hand_brightness,		 // ручной уровень подсветки
		backlight_level,			 // текущий уровень подсветки
		day_brightness,				 // уровень дневной подсвеки
		night_brightness,			 // уровень ночной подсветки
		begin_h,					 // Время ночной подсветки начало, часы
		begin_m,					 // Время ночной подсветки начало, минуты
		end_h,						 // Время ночной подсветки конец, часы
		end_m,						 // Время ночной подсветки конец, минуты
		FanControl,					 // Режим управления оборотами вентилятора
		min_temp,					 // Минимальная температура срабатывания автоматики управления оборотами
		max_temp,					 // Максимальная температура срабатывания биппера и выключения звука
		min_pwm,					 // Минимальный уровень оборотов вентилятора при включении питания
		hand_pwm;					 // Уровень оборотов при ручной регулировке
	bool ldr;						 // Признак присутствия фоторезистора

	char tzone[TZONELEN];
	char ntp_server[4][NTP_LEN];

} radiola_config_s;

typedef struct station_slot
{

	char name[64];	 // Name station
	char domain[73]; // url
	char file[116];	 // path
	uint16_t port;	 // port
	uint8_t fav;	 // FAV
} station_slot_s;

extern radiola_config_s *MainConfig;

esp_err_t open_storage(const char *namespace, nvs_open_mode open_mode, nvs_handle *handle);
void close_storage(nvs_handle handle);
void ConfigInit(bool reset);
void eeEraseAll();
void saveStation(station_slot_s *station, uint8_t ID);
void EraseNameSpace(const char *namespace);
station_slot_s *getStation(uint8_t ID);
void SaveConfig();
uint8_t GetTotalStations();
esp_err_t nvs_get_ir_key(nvs_handle handle, const char *key, uint32_t *out_set1, uint32_t *out_set2);
esp_err_t nvs_set_ir_key(const char *key, char *ir_keys);
