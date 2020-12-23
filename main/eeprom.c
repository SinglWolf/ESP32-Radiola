/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "eeprom.h"
#include "stdio.h"
#include "stdlib.h"
#include <assert.h>

#include "interface.h"

#define PART_HARDWARE "hardware"

const static char *TAG = "eeprom";
// static xSemaphoreHandle muxDevice;
static xSemaphoreHandle muxnvs = NULL;

const esp_partition_t *DEVICE;
const esp_partition_t *DEVICE1;
const esp_partition_t *STATIONS;

radiola_config_s *MainConfig;

esp_err_t open_storage(const char *namespace, nvs_open_mode open_mode, nvs_handle *handle)
{
	esp_err_t err;
	if (muxnvs == NULL)
		muxnvs = xSemaphoreCreateMutex();
	xSemaphoreTake(muxnvs, portMAX_DELAY);
	err = nvs_flash_init_partition(PART_HARDWARE);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "Hardware partition not found");
		return err;
	}
	err = nvs_open_from_partition(PART_HARDWARE, namespace, open_mode, handle);
	if (err != ESP_OK)
	{
		// ESP_LOGD(TAG, "Namespace %s not found, ERR: %x", namespace, err);
		nvs_flash_deinit_partition(PART_HARDWARE);
		xSemaphoreGive(muxnvs);
	}
	return err;
}
void close_storage(nvs_handle handle)
{
	nvs_commit(handle); // if a write is pending
	nvs_close(handle);
	nvs_flash_deinit_partition(PART_HARDWARE);
	xSemaphoreGive(muxnvs);
}

uint8_t GetTotalStations()
{
	//
	ESP_ERROR_CHECK(nvs_flash_init_partition(PART_HARDWARE));
	nvs_iterator_t it = nvs_entry_find(PART_HARDWARE, "stations", NVS_TYPE_BLOB);
	uint8_t counter = 0;
	while (it != NULL)
	{
		// nvs_entry_info_t info;
		// nvs_entry_info(it, &info);
		// ESP_LOGI(TAG, "key '%s', type '%d' \n", info.key, info.type);
		it = nvs_entry_next(it);
		counter++;
	};
	ESP_LOGI(TAG, "Total stations: %d", counter);
	return counter;
}

void ConfigInit(bool reset)
{
	MainConfig = calloc(1, sizeof(radiola_config_s));
	nvs_handle config_handle;

	esp_err_t err = open_storage(MAIN_SPACE, NVS_READONLY, &config_handle);
	if (err == ESP_ERR_NVS_NOT_FOUND || reset)
	{
		close_storage(config_handle);
		if (!reset)
		{
			ESP_LOGW(TAG, "Rariola config not found...");
		}
		else
		{
			ESP_LOGW(TAG, "Forced resetting config the Radiola.");
		}
		MainConfig->options |= Y_WIFIAUTO;		// Автоподключение к Wi-Wi при обрыве соединения
		MainConfig->options &= N_GPIOMODE;		// Режим считывания GPIO 0 - по-умолчанию, 1 - из NVS
		MainConfig->uartspeed = 115200;			// default
		MainConfig->ir_mode = IR_DEFAULD;		// Опрос кодов по-умолчанию
		MainConfig->audio_input_num = COMPUTER; // default
		MainConfig->trace_level = ESP_LOG_INFO; // default
		MainConfig->vol = 100;					// default
		MainConfig->ntp_mode = 0;				// Режим  использования серверов NTP 0 - по-умолчанию, 1 - пользовательские
		MainConfig->options |= Y_ROTAT;
		MainConfig->options |= Y_DDMM;
		MainConfig->lcd_out = 0;
		MainConfig->backlight_mode = BY_LIGHTING;		// по-умолчанию подсветка регулируемая
		MainConfig->backlight_level = 255;				// по-умолчанию подсветка максимальная
		strcpy(MainConfig->tzone, CONFIG_TZONE);		// по-умолчанию часовой пояс Екатеринбурга
		strcpy(MainConfig->ntp_server[0], CONFIG_NTP0); // 0
		strcpy(MainConfig->ntp_server[1], CONFIG_NTP1); // 1
		strcpy(MainConfig->ntp_server[2], CONFIG_NTP2); // 2
		strcpy(MainConfig->ntp_server[3], CONFIG_NTP3); // 3
		MainConfig->current_ap = STA1;

#ifdef CONFIG_WIWI1_PRESENT
		strcpy(MainConfig->ssid1, CONFIG_WIFI_SSID1);
		strcpy(MainConfig->pass1, CONFIG_WIFI_PASSWORD1);
#ifdef CONFIG_DHCP1_PRESENT
		MainConfig->dhcpEn1 = 1;
#else
		MainConfig->dhcpEn1 = 0;
		ip_addr_t tempIP1;
		ipaddr_aton(CONFIG_IPADDRESS1, &tempIP1);
		memcpy(MainConfig->ipAddr1, &tempIP1, sizeof(uint32_t));
		ipaddr_aton(CONFIG_GATEWAY1, &tempIP1);
		memcpy(MainConfig->gate1, &tempIP1, sizeof(uint32_t));
		ipaddr_aton(CONFIG_MASK1, &tempIP1);
		memcpy(MainConfig->mask1, &tempIP1, sizeof(uint32_t));
#endif
#endif

#ifdef CONFIG_WIWI2_PRESENT
		strcpy(MainConfig->ssid2, CONFIG_WIFI_SSID2);
		strcpy(MainConfig->pass2, CONFIG_WIFI_PASSWORD2);
#ifdef CONFIG_DHCP2_PRESENT
		MainConfig->dhcpEn2 = 1;
#else
		MainConfig->dhcpEn2 = 0;
		ip_addr_t tempIP2;
		ipaddr_aton(CONFIG_IPADDRESS2, &tempIP2);
		memcpy(MainConfig->ipAddr2, &tempIP2, sizeof(uint32_t));
		ipaddr_aton(CONFIG_GATEWAY2, &tempIP2);
		memcpy(MainConfig->gate2, &tempIP2, sizeof(uint32_t));
		ipaddr_aton(CONFIG_MASK2, &tempIP2);
		memcpy(MainConfig->mask2, &tempIP2, sizeof(uint32_t));
#endif
#endif
		ESP_LOGW(TAG, "Save def Radiola config.");
		SaveConfig();
		return;
	}
	else if (err == ESP_OK)
	{
		size_t required_size = sizeof(radiola_config_s);
		ESP_ERROR_CHECK(nvs_get_blob(config_handle, "config", MainConfig, &required_size));
		close_storage(config_handle);
	}
	else
	{
		ESP_LOGE(TAG, "get Radiola config fail, err No: 0x%x", err);
	}
	ESP_LOGI(TAG, "Main config init done...");
}

void eeEraseAll()
{
	ConfigInit(true);

	EraseNameSpace(STATION_SPACE);
	EraseNameSpace(GPIOS_SPACE);
	EraseNameSpace(IRCODE_SPACE);

	if (tda7313_get_present())
		ESP_ERROR_CHECK(tda7313_init_nvs(true));
	kprintf("#erase All done##\n");
	fflush(stdout);
	vTaskDelay(100);
	esp_restart();
}

void EraseNameSpace(const char *namespace)
{
	nvs_handle erase_handle;

	// esp_err_t err = open_storage(namespace, NVS_READWRITE, &erase_handle);
	esp_err_t err = open_storage(namespace, NVS_READONLY, &erase_handle);
	close_storage(erase_handle);
	if (err == ESP_OK)
	{
		err = open_storage(namespace, NVS_READWRITE, &erase_handle);
		err |= nvs_erase_all(erase_handle);
		close_storage(erase_handle);
	}

	if (err != ESP_OK)
	{
		if (err == ESP_ERR_NVS_NOT_FOUND)
			ESP_LOGI(TAG, "Namespace: %s not yet recorded\n", namespace);
		else
			ESP_LOGE(TAG, "Erase namespace: %s fails, err No: 0x%x", namespace, err);
	}
	else
	{
		ESP_LOGI(TAG, "Erase namespace: %s OK.", namespace);
		MainConfig->TotalStations = 0;
		SaveConfig();
	}
}

void saveStation(station_slot_s *station, uint8_t ID)
{
	if (ID > MAXSTATIONS - 1)
	{
		ESP_LOGE(TAG, "ID: %u fails!", ID);
		return;
	}
	nvs_handle station_handle;

	esp_err_t err = open_storage(STATION_SPACE, NVS_READWRITE, &station_handle);
	if (err == ESP_OK)
	{
		char id[6];
		sprintf(id, "ID%u", ID);
		size_t required_size = sizeof(station_slot_s);
		ESP_ERROR_CHECK(nvs_set_blob(station_handle, id, station, required_size));
	}
	else
	{
		ESP_LOGE(TAG, "saveStation fails, err No: 0x%x", err);
	}
	close_storage(station_handle);
}

station_slot_s *getStation(uint8_t ID)
{
	if (ID > MainConfig->TotalStations)
	{
		ESP_LOGE(TAG, "getStation fails pos=%d\n", ID);
		return NULL;
	}

	nvs_handle station_handle;

	esp_err_t err = open_storage(STATION_SPACE, NVS_READONLY, &station_handle);
	station_slot_s *slot = calloc(1, sizeof(station_slot_s));
	if (err == ESP_OK)
	{
		char id[13];
		sprintf(id, "ID%u", ID);
		size_t required_size = sizeof(station_slot_s);
		err = (nvs_get_blob(station_handle, id, slot, &required_size));
	}
	if (err != ESP_OK)
	{
		if (err == ESP_ERR_NVS_NOT_FOUND)
			ESP_LOGI(TAG, "Station not yet recorded\n");
		else
			ESP_LOGE(TAG, "getStation fails, err No: 0x%x", err);
		slot = NULL;
	}

	close_storage(station_handle);

	return slot;
	//
}

void SaveConfig()
{
	nvs_handle config_handle;
	size_t required_size = sizeof(radiola_config_s);
	esp_err_t err = open_storage(MAIN_SPACE, NVS_READWRITE, &config_handle);
	if (err == ESP_OK)
	{
		ESP_ERROR_CHECK(nvs_set_blob(config_handle, "config", MainConfig, required_size));
		close_storage(config_handle);
		if (tda7313_get_present())
			ESP_ERROR_CHECK(tda7313_save_nvs());
	}
	else
	{
		ESP_LOGE(TAG, "Save Radiola config fail, err No: 0x%x", err);
	}
}

esp_err_t nvs_get_ir_key(nvs_handle handle, const char *key, uint32_t *out_set1, uint32_t *out_set2)
{
	// init default
	esp_err_t err = ESP_OK;
	*out_set1 = 0;
	*out_set2 = 0;
	size_t required_size;
	err |= nvs_get_str(handle, key, NULL, &required_size);
	if (required_size > 1)
	{
		char *string = malloc(required_size);
		err |= nvs_get_str(handle, key, string, &required_size);
		sscanf(string, "%x %x", out_set1, out_set2);
		free(string);
	}
	ESP_LOGD(TAG, "Load from NVS Key: %s, set1: %x, set2: %x, err: %d", key, *out_set1, *out_set2, err);

	return err;
}

esp_err_t nvs_set_ir_key(const char *key, char *ir_keys)
{
	nvs_handle gpio_handle;
	esp_err_t err = open_storage(GPIOS_SPACE, NVS_READWRITE, &gpio_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_buzzer");
		return err;
	}

	err = nvs_set_str(gpio_handle, key, ir_keys);
	ESP_LOGD(TAG, "Save Key: %s, ir_keys: %s, err: %d", key, ir_keys, err);
	close_storage(gpio_handle);
	return err;
}
