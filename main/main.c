/*
  ESP32-Radiola
  A WiFi webradio player and Media Centre
  Based on source code of  Ka-Radio32 Copyright (C) 2017  KaraWin
  Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "main.h"
#include "esp_ota_ops.h"
#include "mdns.h"

#include "spiram_fifo.h"
#include "audio_player.h"

#include "eeprom.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "gpios.h"
#include "servers.h"
#include "webclient.h"
#include "webserver.h"
#include "interface.h"
#include "vs1053.h"
#include "ClickEncoder.h"
#include "addon.h"
#include "tda7313.h"
#include "custom.h"
#include "bt_x01.h"

#include "sdkconfig.h"

/* The event group allows multiple bits for each event*/
//   are we connected  to the AP with an IP? */
const int CONNECTED_BIT = 0x00000001;
//
const int CONNECTED_AP = 0x00000010;

#define TAG "main"

//Priorities of the reader and the decoder thread. bigger number = higher prio
#define PRIO_READER configMAX_PRIORITIES - 3
#define PRIO_MQTT configMAX_PRIORITIES - 3
#define PRIO_CONNECT configMAX_PRIORITIES - 1
#define striWATERMARK "watermark: %d  heap: %d"

static void obtain_time(void);
static void initialize_sntp(void);
void start_network();
void autoPlay();
/* */
static bool wifiInitDone = false;
static EventGroupHandle_t wifi_event_group;
xQueueHandle event_queue;

//xSemaphoreHandle print_mux;
player_t *player_config;
static uint8_t clientIvol = 0;
//ip
static char localIp[20];
// 4MB sram?
static bool bigRam, time_sync = false;
// timeout to save volume in flash
static uint32_t ctimeVol = 0;
static bool divide = false;
// disable 1MS timer interrupt
IRAM_ATTR void noInterrupt1Ms() { timer_disable_intr(TIMERGROUP1MS, msTimer); }
// enable 1MS timer interrupt
IRAM_ATTR void interrupt1Ms() { timer_enable_intr(TIMERGROUP1MS, msTimer); }
//IRAM_ATTR void noInterrupts() {noInterrupt1Ms();}
//IRAM_ATTR void interrupts() {interrupt1Ms();}

IRAM_ATTR char *getIp() { return (localIp); }
IRAM_ATTR uint8_t getIvol() { return clientIvol; }
IRAM_ATTR void setIvol(uint8_t vol)
{
	clientIvol = vol;
	ctimeVol = 0;
}

/*
IRAM_ATTR void   microsCallback(void *pArg) {
	int timer_idx = (int) pArg;
	queue_event_t evt;	
	TIMERG1.hw_timer[timer_idx].update = 1;
	TIMERG1.int_clr_timers.t1 = 1; //isr ack
		evt.type = TIMER_1mS;
        evt.i1 = TIMERGROUP1mS;
        evt.i2 = timer_idx;
	xQueueSendFromISR(event_queue, &evt, NULL);	
	TIMERG1.hw_timer[timer_idx].config.alarm_en = 1;
}*/
//
IRAM_ATTR bool bigSram() { return bigRam; }
//-----------------------------------
// every 500µs
IRAM_ATTR void msCallback(void *pArg)
{
	int timer_idx = (int)pArg;

	// queue_event_t evt;
	TIMERG1.hw_timer[timer_idx].update = 1;
	TIMERG1.int_clr_timers.t0 = 1; //isr ack
	if (divide)
	{
		ctimeVol++; // to save volume
	}
	divide = !divide;
	if (serviceAddon != NULL)
		serviceAddon(); // for the encoders and buttons
	TIMERG1.hw_timer[timer_idx].config.alarm_en = 1;
}

IRAM_ATTR void sleepCallback(void *pArg)
{
	int timer_idx = (int)pArg;
	queue_event_t evt;
	TIMERG0.int_clr_timers.t0 = 1; //isr ack
	evt.type = TIMER_SLEEP;
	evt.i1 = TIMERGROUP;
	evt.i2 = timer_idx;
	xQueueSendFromISR(event_queue, &evt, NULL);
	TIMERG0.hw_timer[timer_idx].config.alarm_en = 0;
}
IRAM_ATTR void wakeCallback(void *pArg)
{

	int timer_idx = (int)pArg;
	queue_event_t evt;
	TIMERG0.int_clr_timers.t1 = 1;
	evt.i1 = TIMERGROUP;
	evt.i2 = timer_idx;
	evt.type = TIMER_WAKE;
	xQueueSendFromISR(event_queue, &evt, NULL);
	TIMERG0.hw_timer[timer_idx].config.alarm_en = 0;
}

void startSleep(uint32_t delay)
{
	ESP_LOGD(TAG, "Delay:%d\n", delay);
	if (delay == 0)
		return;
	ESP_ERROR_CHECK(timer_set_counter_value(TIMERGROUP, sleepTimer, 0x00000000ULL));
	ESP_ERROR_CHECK(timer_set_alarm_value(TIMERGROUP, sleepTimer, TIMERVALUE(delay * 60)));
	ESP_ERROR_CHECK(timer_enable_intr(TIMERGROUP, sleepTimer));
	ESP_ERROR_CHECK(timer_set_alarm(TIMERGROUP, sleepTimer, TIMER_ALARM_EN));
	ESP_ERROR_CHECK(timer_start(TIMERGROUP, sleepTimer));
}
void stopSleep()
{
	ESP_LOGD(TAG, "stopDelayDelay\n");
	ESP_ERROR_CHECK(timer_pause(TIMERGROUP, sleepTimer));
}
void startWake(uint32_t delay)
{
	ESP_LOGD(TAG, "Wake Delay:%d\n", delay);
	if (delay == 0)
		return;
	ESP_ERROR_CHECK(timer_set_counter_value(TIMERGROUP, wakeTimer, 0x00000000ULL));
	//TIMER_INTERVAL0_SEC * TIMER_SCALE - TIMER_FINE_ADJ
	ESP_ERROR_CHECK(timer_set_alarm_value(TIMERGROUP, wakeTimer, TIMERVALUE(delay * 60)));
	ESP_ERROR_CHECK(timer_enable_intr(TIMERGROUP, wakeTimer));
	ESP_ERROR_CHECK(timer_set_alarm(TIMERGROUP, wakeTimer, TIMER_ALARM_EN));
	ESP_ERROR_CHECK(timer_start(TIMERGROUP, wakeTimer));
}
void stopWake()
{
	ESP_LOGD(TAG, "stopDelayWake\n");
	ESP_ERROR_CHECK(timer_pause(TIMERGROUP, wakeTimer));
}

void initTimers()
{
	timer_config_t config;
	config.alarm_en = 1;
	config.auto_reload = TIMER_AUTORELOAD_DIS;
	config.counter_dir = TIMER_COUNT_UP;
	config.divider = TIMER_DIVIDER;
	config.intr_type = TIMER_INTR_LEVEL;
	config.counter_en = TIMER_PAUSE;

	/*Configure timer sleep*/
	ESP_ERROR_CHECK(timer_init(TIMERGROUP, sleepTimer, &config));
	ESP_ERROR_CHECK(timer_pause(TIMERGROUP, sleepTimer));
	ESP_ERROR_CHECK(timer_isr_register(TIMERGROUP, sleepTimer, sleepCallback, (void *)sleepTimer, 0, NULL));
	/*Configure timer wake*/
	ESP_ERROR_CHECK(timer_init(TIMERGROUP, wakeTimer, &config));
	ESP_ERROR_CHECK(timer_pause(TIMERGROUP, wakeTimer));
	ESP_ERROR_CHECK(timer_isr_register(TIMERGROUP, wakeTimer, wakeCallback, (void *)wakeTimer, 0, NULL));
	/*Configure timer 1MS*/
	config.auto_reload = TIMER_AUTORELOAD_EN;
	config.divider = TIMER_DIVIDER1MS;
	ESP_ERROR_CHECK(timer_init(TIMERGROUP1MS, msTimer, &config));
	ESP_ERROR_CHECK(timer_pause(TIMERGROUP1MS, msTimer));
	ESP_ERROR_CHECK(timer_isr_register(TIMERGROUP1MS, msTimer, msCallback, (void *)msTimer, 0, NULL));
	/* start 1MS timer*/
	ESP_ERROR_CHECK(timer_set_counter_value(TIMERGROUP1MS, msTimer, 0x00000000ULL));
	ESP_ERROR_CHECK(timer_set_alarm_value(TIMERGROUP1MS, msTimer, TIMERVALUE1MS(1)));
	ESP_ERROR_CHECK(timer_enable_intr(TIMERGROUP1MS, msTimer));
	ESP_ERROR_CHECK(timer_set_alarm(TIMERGROUP1MS, msTimer, TIMER_ALARM_EN));
	ESP_ERROR_CHECK(timer_start(TIMERGROUP1MS, msTimer));

	/*Configure timer 1µS*/
	/*	config.auto_reload = TIMER_AUTORELOAD_EN;
	config.divider = TIMER_DIVIDER1mS;
	ESP_ERROR_CHECK(timer_init(TIMERGROUP1mS, microsTimer, &config));
	ESP_ERROR_CHECK(timer_pause(TIMERGROUP1mS, microsTimer));
	ESP_ERROR_CHECK(timer_isr_register(TIMERGROUP1mS, microsTimer, microsCallback, (void*) microsTimer, 0, NULL));
*/
	/* start 1µS timer*/
	/*	ESP_ERROR_CHECK(timer_set_counter_value(TIMERGROUP1mS, microsTimer, 0x00000000ULL));
	ESP_ERROR_CHECK(timer_set_alarm_value(TIMERGROUP1mS, microsTimer,TIMERVALUE1mS(10))); // 10 ms timer
	ESP_ERROR_CHECK(timer_enable_intr(TIMERGROUP1mS, microsTimer));
	ESP_ERROR_CHECK(timer_set_alarm(TIMERGROUP1mS, microsTimer,TIMER_ALARM_EN));
	ESP_ERROR_CHECK(timer_start(TIMERGROUP1mS, microsTimer));*/
}

//////////////////////////////////////////////////////////////////

/******************************************************************************
 * FunctionName : checkUart
 * Description  : Check for a valid uart baudrate
 * Parameters   : baud
 * Returns      : baud
*******************************************************************************/
uint32_t checkUart(uint32_t speed)
{
	uint32_t valid[] = {1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76880, 115200, 230400};
	int i;
	for (i = 0; i < 12; i++)
	{
		if (speed == valid[i])
			return speed;
	}
	return 115200; // default
}

/******************************************************************************
 * FunctionName : init_hardware
 * Description  : Init all hardware, partitions etc
 *******************************************************************************/
static void init_hardware()
{
	// Настройка тахометра
	tach_init();
	// Настройка термометра
	init_ds18b20();
	// Настройка TDA7313...
	if (tda7313_init() == ESP_OK)
	{
		ESP_ERROR_CHECK(tda7313_init_nvs(false));
		ESP_ERROR_CHECK(tda7313_set_input(g_device->audio_input_num));
	}
	if (VS1053_HW_init()) // init spi
	{
		VS1053_Start();
	}
	ESP_LOGI(TAG, "hardware initialized");
}

/* event handler for pre-defined wifi events */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
							   int32_t event_id, void *event_data)
{
	//EventGroupHandle_t wifi_event = ctx;

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		/* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
		xEventGroupClearBits(wifi_event_group, CONNECTED_AP);
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		ESP_LOGE(TAG, "Wifi Disconnected.");
		vTaskDelay(10);
		if (!getAutoWifi() && (wifiInitDone))
		{
			ESP_LOGE(TAG, "reboot");
			vTaskDelay(10);
			esp_restart();
		}
		else
		{
			if (wifiInitDone) // a completed init done
			{
				ESP_LOGE(TAG, "Connection tried again");
				//clientDisconnect("Wifi Disconnected.");
				clientSilentDisconnect();
				vTaskDelay(100);
				clientSaveOneHeader("Wifi Disconnected.", 18, METANAME);
				vTaskDelay(100);
				while (esp_wifi_connect() == ESP_ERR_WIFI_SSID)
					vTaskDelay(10);
			}
			else
			{
				ESP_LOGE(TAG, "Try next AP");
				vTaskDelay(10);
			} // init failed?
		}
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
	{
		xEventGroupSetBits(wifi_event_group, CONNECTED_AP);
		ESP_LOGI(TAG, "Wifi connected");
		if (wifiInitDone)
		{
			clientSaveOneHeader("Wifi Connected.", 18, METANAME);
			vTaskDelay(200);
			autoPlay();
		} // retry
		else
		{
			wifiInitDone = true;
		}
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
	}
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
	{
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		xEventGroupSetBits(wifi_event_group, CONNECTED_AP);
		wifiInitDone = true;
	}
}
static void unParse(char *str)
{
	int i;
	if (str == NULL)
		return;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == '\\')
		{
			str[i] = str[i + 1];
			str[i + 1] = 0;
			if (str[i + 2] != 0)
				strcat(str, str + i + 2);
		}
	}
}

static void start_wifi()
{
	ESP_LOGI(TAG, "Starting WiFi");

	setAutoWifi();

	char ssid[SSIDLEN];
	char pass[PASSLEN];
	static bool first_pass = false;

	static bool initialized = false;
	if (!initialized)
	{
		esp_netif_init();
		wifi_event_group = xEventGroupCreate();
		ESP_ERROR_CHECK(esp_event_loop_create_default());
		ap = esp_netif_create_default_wifi_ap();
		assert(ap);
		sta = esp_netif_create_default_wifi_sta();
		assert(sta);
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));
		ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
		ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
		//ESP_ERROR_CHECK( esp_wifi_start() );
		initialized = true;
	}
	ESP_LOGI(TAG, "WiFi init done!");

	if (g_device->current_ap == APMODE)
	{
		if (strlen(g_device->ssid1) != 0)
		{
			g_device->current_ap = STA1;
		}
		else
		{
			if (strlen(g_device->ssid2) != 0)
			{
				g_device->current_ap = STA2;
			}
			else
				g_device->current_ap = APMODE;
		}
		saveDeviceSettings(g_device);
	}

	while (1)
	{
		if (first_pass)
		{
			ESP_ERROR_CHECK(esp_wifi_stop());
			vTaskDelay(5);
		}
		switch (g_device->current_ap)
		{
		case STA1: //ssid1 used
			strcpy(ssid, g_device->ssid1);
			strcpy(pass, g_device->pass1);
			esp_wifi_set_mode(WIFI_MODE_STA);
			break;
		case STA2: //ssid2 used
			strcpy(ssid, g_device->ssid2);
			strcpy(pass, g_device->pass2);
			esp_wifi_set_mode(WIFI_MODE_STA);
			break;

		default: // other: AP mode
			g_device->current_ap = APMODE;
			ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
		}

		if (g_device->current_ap == APMODE)
		{
			printf("WIFI GO TO AP MODE\n");
			wifi_config_t ap_config = {
				.ap = {
					.ssid = "WifiESP32Radiola",
					.authmode = WIFI_AUTH_OPEN,
					.max_connection = 2,
					.beacon_interval = 200},
			};
			ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
			ESP_LOGE(TAG, "The default AP is  WifiESP32Radiola. Connect your wifi to it.\nThen connect a webbrowser to 192.168.4.1 and go to Setting\nMay be long to load the first time.Be patient.");

			vTaskDelay(1);
			ESP_ERROR_CHECK(esp_wifi_start());

			g_device->audio_input_num = COMPUTER;
		}
		else
		{
			printf("WIFI TRYING TO CONNECT TO SSID %d\n", g_device->current_ap);
			wifi_config_t wifi_config =
				{
					.sta =
						{
							.bssid_set = 0,
						},
				};
			strcpy((char *)wifi_config.sta.ssid, ssid);
			strcpy((char *)wifi_config.sta.password, pass);
			unParse((char *)(wifi_config.sta.ssid));
			unParse((char *)(wifi_config.sta.password));
			if (strlen(ssid) /*&&strlen(pass)*/)
			{
				if (CONNECTED_BIT > 1)
				{
					esp_wifi_disconnect();
				}
				ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
				//	ESP_LOGI(TAG, "connecting %s, %d, %s, %d",ssid,strlen((char*)(wifi_config.sta.ssid)),pass,strlen((char*)(wifi_config.sta.password)));
				ESP_LOGI(TAG, "connecting %s", ssid);
				ESP_ERROR_CHECK(esp_wifi_start());
			}
			else
			{
				g_device->current_ap++;
				g_device->current_ap %= 3;

				if (getAutoWifi() && (g_device->current_ap == APMODE))
				{
					g_device->current_ap = STA1; // if autoWifi then wait for a reconnection to an AP
					printf("Wait for the AP\n");
				}
				else
				{
					printf("Empty AP. Try next one\n");
				}

				saveDeviceSettings(g_device);
				continue;
			}
		}

		/* Wait for the callback to set the CONNECTED_AP in the event group. */
		if ((xEventGroupWaitBits(wifi_event_group, CONNECTED_AP, false, true, 2000) & CONNECTED_AP) == 0)
		//timeout . Try the next AP
		{
			g_device->current_ap++;
			g_device->current_ap %= 3;
			saveDeviceSettings(g_device);
			printf("\ndevice->current_ap: %d\n", g_device->current_ap);
		}
		else
			break; //
		first_pass = true;
	}
}

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Notification of a time synchronization event");
	time_sync = true;
}

static void obtain_time(void)
{
	if (sntp_enabled())
		sntp_stop();
	initialize_sntp();

	// wait for time to be set
	time_t now = 0;
	struct tm timeinfo = {0};
	int retry = 0;
	const int retry_count = 30;
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
	{
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	time(&now);
	localtime_r(&now, &timeinfo);
}

static void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, g_device->ntp_server[0]);
	sntp_setservername(1, g_device->ntp_server[1]);
	sntp_setservername(2, g_device->ntp_server[2]);
	sntp_setservername(3, g_device->ntp_server[3]);
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
	sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
	sntp_init();
}

void start_network()
{
	//	struct device_settings *g_device;

	esp_netif_ip_info_t info;
	wifi_mode_t mode;
	ip4_addr_t ipAddr;
	ip4_addr_t mask;
	ip4_addr_t gate;
	uint8_t dhcpEn = 0;

	IP4_ADDR(&ipAddr, 192, 168, 4, 1);
	IP4_ADDR(&gate, 192, 168, 4, 1);
	IP4_ADDR(&mask, 255, 255, 255, 0);

	esp_netif_dhcpc_stop(sta); // Don't run a DHCP client

	switch (g_device->current_ap)
	{
	case STA1: //ssid1 used
		IP4_ADDR(&ipAddr, g_device->ipAddr1[0], g_device->ipAddr1[1], g_device->ipAddr1[2], g_device->ipAddr1[3]);
		IP4_ADDR(&gate, g_device->gate1[0], g_device->gate1[1], g_device->gate1[2], g_device->gate1[3]);
		IP4_ADDR(&mask, g_device->mask1[0], g_device->mask1[1], g_device->mask1[2], g_device->mask1[3]);
		dhcpEn = g_device->dhcpEn1;
		break;
	case STA2: //ssid2 used
		IP4_ADDR(&ipAddr, g_device->ipAddr2[0], g_device->ipAddr2[1], g_device->ipAddr2[2], g_device->ipAddr2[3]);
		IP4_ADDR(&gate, g_device->gate2[0], g_device->gate2[1], g_device->gate2[2], g_device->gate2[3]);
		IP4_ADDR(&mask, g_device->mask2[0], g_device->mask2[1], g_device->mask2[2], g_device->mask2[3]);
		dhcpEn = g_device->dhcpEn2;
		break;
	}

	ip4_addr_copy(info.ip, ipAddr);
	ip4_addr_copy(info.gw, gate);
	ip4_addr_copy(info.netmask, mask);

	ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));

	if (mode == WIFI_MODE_AP)
	{
		xEventGroupWaitBits(wifi_event_group, CONNECTED_AP, false, true, 3000);
		ip4_addr_copy(info.ip, ipAddr);
		esp_netif_set_ip_info(ap, &info);

		esp_netif_ip_info_t ap_ip_info;
		ap_ip_info.ip.addr = 0;
		while (ap_ip_info.ip.addr == 0)
		{
			esp_netif_get_ip_info(ap, &ap_ip_info);
		}
	}
	else // mode STA
	{
		if (dhcpEn) // check if ip is valid without dhcp
		{
			esp_netif_dhcpc_start(sta); //  run a DHCP client
		}
		else
		{
			ESP_ERROR_CHECK(esp_netif_set_ip_info(sta, &info));
			dns_clear_servers(false);
			IP_SET_TYPE((ip_addr_t *)&info.gw, IPADDR_TYPE_V4); // mandatory
			((ip_addr_t *)&info.gw)->type = IPADDR_TYPE_V4;
			dns_setserver(0, (ip_addr_t *)&info.gw);
			dns_setserver(1, (ip_addr_t *)&info.gw); // if static ip	check dns
		}

		// wait for ip
		if ((xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 3000) & CONNECTED_BIT) == 0) //timeout
		{																									// enable dhcp and restart
			if (g_device->current_ap == 1)
			{
				g_device->dhcpEn1 = 1;
			}
			else
			{
				g_device->dhcpEn2 = 1;
			}
			saveDeviceSettings(g_device);
			fflush(stdout);
			vTaskDelay(100);
			esp_restart();
		}

		vTaskDelay(1);

		// retrieve the current ip
		esp_netif_ip_info_t sta_ip_info;
		sta_ip_info.ip.addr = 0;
		while (sta_ip_info.ip.addr == 0)
		{
			esp_netif_get_ip_info(sta, &sta_ip_info);
		}

		const ip_addr_t *ipdns0 = dns_getserver(0);
		ESP_LOGW(TAG, "DNS: %s  \n", ip4addr_ntoa((struct ip4_addr *)&ipdns0));

		if (dhcpEn) // if dhcp enabled update fields
		{
			esp_netif_get_ip_info(sta, &info);
			switch (g_device->current_ap)
			{
			case STA1: //ssid1 used
				ip4_addr_copy(ipAddr, info.ip);
				g_device->ipAddr1[0] = ip4_addr1_val(ipAddr);
				g_device->ipAddr1[1] = ip4_addr2_val(ipAddr);
				g_device->ipAddr1[2] = ip4_addr3_val(ipAddr);
				g_device->ipAddr1[3] = ip4_addr4_val(ipAddr);

				ip4_addr_copy(ipAddr, info.gw);
				g_device->gate1[0] = ip4_addr1_val(ipAddr);
				g_device->gate1[1] = ip4_addr2_val(ipAddr);
				g_device->gate1[2] = ip4_addr3_val(ipAddr);
				g_device->gate1[3] = ip4_addr4_val(ipAddr);

				ip4_addr_copy(ipAddr, info.netmask);
				g_device->mask1[0] = ip4_addr1_val(ipAddr);
				g_device->mask1[1] = ip4_addr2_val(ipAddr);
				g_device->mask1[2] = ip4_addr3_val(ipAddr);
				g_device->mask1[3] = ip4_addr4_val(ipAddr);
				break;

			case STA2: //ssid2 used
				ip4_addr_copy(ipAddr, info.ip);
				g_device->ipAddr2[0] = ip4_addr1_val(ipAddr);
				g_device->ipAddr2[1] = ip4_addr2_val(ipAddr);
				g_device->ipAddr2[2] = ip4_addr3_val(ipAddr);
				g_device->ipAddr2[3] = ip4_addr4_val(ipAddr);

				ip4_addr_copy(ipAddr, info.gw);
				g_device->gate2[0] = ip4_addr1_val(ipAddr);
				g_device->gate2[1] = ip4_addr2_val(ipAddr);
				g_device->gate2[2] = ip4_addr3_val(ipAddr);
				g_device->gate2[3] = ip4_addr4_val(ipAddr);

				ip4_addr_copy(ipAddr, info.netmask);
				g_device->mask2[0] = ip4_addr1_val(ipAddr);
				g_device->mask2[1] = ip4_addr2_val(ipAddr);
				g_device->mask2[2] = ip4_addr3_val(ipAddr);
				g_device->mask2[3] = ip4_addr4_val(ipAddr);
				break;
			}
		}
		saveDeviceSettings(g_device);
		esp_netif_set_hostname(sta, "ESP32Radiola");

		// wait for time to be set
		lcd_welcome("", "ПОЛУЧЕНИЕ ВРЕМЕНИ");

		time_t now = 0;
		struct tm timeinfo = {0};
		time(&now);
		localtime_r(&now, &timeinfo);
		// Is time set? If not, tm_year will be (1970 - 1900).
		if (timeinfo.tm_year < (2016 - 1900))
		{
			ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
			obtain_time();
			// update 'now' variable with current time
			time(&now);
		}
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
		else
		{
			// add 500 ms error to the current system time.
			// Only to demonstrate a work of adjusting method!
			{
				ESP_LOGI(TAG, "Add a error for test adjtime");
				struct timeval tv_now;
				gettimeofday(&tv_now, NULL);
				int64_t cpu_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
				int64_t error_time = cpu_time + 500 * 1000L;
				struct timeval tv_error = {.tv_sec = error_time / 1000000L, .tv_usec = error_time % 1000000L};
				settimeofday(&tv_error, NULL);
			}

			ESP_LOGI(TAG, "Time was set, now just adjusting it. Use SMOOTH SYNC method.");
			obtain_time();
			// update 'now' variable with current time
			time(&now);
		}
#endif
		if (time_sync)
		{
			lcd_welcome("", "ВРЕМЯ ПОЛУЧЕНО");
		}
		else
		{
			lcd_welcome("", "ВРЕМЯ НЕ ПОЛУЧЕНО");
			// fflush(stdout);
			vTaskDelay(500);
			// esp_restart();
		}
		setenv("TZ", g_device->tzone, 1);
		tzset();
		localtime_r(&now, &timeinfo);
	}
	ip4_addr_copy(ipAddr, info.ip);
	strcpy(localIp, ip4addr_ntoa(&ipAddr));
	ESP_LOGW(TAG, "IP: %s\n\n", localIp);

	lcd_welcome(localIp, "IP ПОЛУЧЕН");
	vTaskDelay(100);
}

//timer isr
void timerTask(void *p)
{

	initTimers();

	queue_event_t evt;

	while (1)
	{
		// read and treat the timer queue events
		// int nb = uxQueueMessagesWaiting(event_queue);
		// if (nb >29) printf(" %d\n",nb);
		while (xQueueReceive(event_queue, &evt, 0))
		{
			switch (evt.type)
			{
			case TIMER_SLEEP:
				clientDisconnect("Timer"); // stop the player
				break;
			case TIMER_WAKE:
				clientConnect(); // start the player
				break;
			// case TIMER_1MS: // 1 ms
			// ctimeVol++; // to save volume
			// break;
			// case TIMER_1mS:  //1µs
			// break;
			default:
				break;
			}
		}

		if (ctimeVol >= TEMPO_SAVE_VOL)
		{
			if (g_device->vol != getIvol())
			{
				g_device->vol = getIvol();
				saveDeviceSettingsVolume(g_device);
				// ESP_LOGD("timerTask",striWATERMARK,uxTaskGetStackHighWaterMark( NULL ),xPortGetFreeHeapSize( ));
			}
			ctimeVol = 0;
		}
		vTaskDelay(1);
	}
	// printf("t0 end\n");

	vTaskDelete(NULL); // stop the task (never reached)
}

void uartInterfaceTask(void *pvParameters)
{
	char tmp[255];
	int d;
	uint8_t c;
	int t;
	// struct device_settings *device;
	uint32_t uspeed;

	uspeed = g_device->uartspeed;
	uart_config_t uart_config0 = {
		.baud_rate = uspeed,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // UART_HW_FLOWCTRL_CTS_RTS,
		.rx_flow_ctrl_thresh = 0,
	};
	uart_param_config(UART_NUM_0, &uart_config0);
	uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

	for (t = 0; t < sizeof(tmp); t++)
		tmp[t] = 0;
	t = 0;

	while (1)
	{
		while (1)
		{
			d = uart_read_bytes(UART_NUM_0, &c, 1, 100);
			if (d > 0)
			{
				if ((char)c == '\r')
					break;
				if ((char)c == '\n')
					break;
				tmp[t] = (char)c;
				t++;
				if (t == sizeof(tmp) - 1)
					t = 0;
			}
			// else printf("uart d: %d, T= %d\n",d,t);
			// switchCommand() ;  // hardware panel of command
		}
		checkCommand(t, tmp);

		// int uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
		// ESP_LOGD("uartInterfaceTask", striWATERMARK, uxHighWaterMark, xPortGetFreeHeapSize());

		for (t = 0; t < sizeof(tmp); t++)
			tmp[t] = 0;
		t = 0;
	}
}

// In STA mode start a station or start in pause mode.
// Show ip on AP mode.
void autoPlay()
{
	char *apmode = "IP %s";
	char *confAP = "Веб-интерфейс доступен по";
	char buf[strlen(apmode) + strlen(confAP)];

	sprintf(buf, apmode, localIp);
	if (g_device->current_ap == APMODE)
	{
		clientSaveOneHeader(confAP, strlen(confAP), METANAME);
		clientSaveOneHeader(buf, strlen(buf), METAGENRE);
	}
	else
	{
		clientSaveOneHeader(buf, strlen(buf), METANAME);

		setCurrentStation(g_device->currentstation);
		if ((g_device->autostart == 1) && (g_device->currentstation != 0xFFFF))
		{
			kprintf("autostart: playing:%d, currentstation:%d\n", g_device->autostart, g_device->currentstation);
			vTaskDelay(50); // wait a bit
			playStationInt(g_device->currentstation);
		}
		else
			clientSaveOneHeader("ОЖИДАНИЕ", 17, METANAME);
	}
}

/**
 * Main entry point
 */

void app_main()
{
	// Затычка для вентилятора
	gpio_pad_select_gpio(PIN_NUM_PWM);
	gpio_set_direction(PIN_NUM_PWM, GPIO_MODE_OUTPUT);
	gpio_set_level(PIN_NUM_PWM, LOW);
	//
	uint32_t uspeed;
	xTaskHandle pxCreatedTask;
	esp_err_t err;

	ESP_LOGI(TAG, "starting app_main()");
	ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());
	// Version infos
	ESP_LOGI(TAG, "Release %s, Revision %s", RELEASE, REVISION);
	ESP_LOGI(TAG, "SDK %s", esp_get_idf_version());
	ESP_LOGI(TAG, "Heap size: %d", xPortGetFreeHeapSize());

	const esp_partition_t *running = esp_ota_get_running_partition();
	ESP_LOGE(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
			 running->type, running->subtype, running->address);
	// Initialize NVS.
	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		// OTA app partition table has a smaller NVS partition size than the non-OTA
		// partition table. This size mismatch may cause NVS initialization to fail.
		// If this happens, we erase NVS partition and initialize NVS again.
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
	// Check if we are in large Sram config
	if (xPortGetFreeHeapSize() > 0x80000)
		bigRam = true;
	//init hardware
	partitions_init();
	ESP_LOGI(TAG, "Partition init done...");
	//
	if (g_device->cleared != 0xAABB)
	{
		ESP_LOGE(TAG, "Device config not ok. Try to restore");
		free(g_device);
		restoreDeviceSettings(); // try to restore the config from the saved one
		g_device = getDeviceSettings();
		if (g_device->cleared != 0xAABB)
		{
			ESP_LOGE(TAG, "Device config not cleared. Clear it.");
			free(g_device);
			eeEraseAll();
			g_device = getDeviceSettings();
			g_device->cleared = 0xAABB;			  // marker init done
			g_device->options &= N_GPIOMODE;	  // Режим считывания GPIO 0 - по-умолчанию, 1 - из NVS
			g_device->uartspeed = 115200;		  // default
			g_device->ir_mode = IR_DEFAULD;		  // Опрос кодов по-умолчанию
			g_device->audio_input_num = COMPUTER; // default
			g_device->trace_level = ESP_LOG_INFO; // default
			g_device->vol = 100;				  // default
			g_device->ntp_mode = 0;				  // Режим  использования серверов NTP 0 - по-умолчанию, 1 - пользовательские
			g_device->options |= Y_ROTAT;
			g_device->options |= Y_DDMM;
			g_device->current_ap = STA1;
			strcpy(g_device->ssid1, CONFIG_WIFI_SSID);
			strcpy(g_device->pass1, CONFIG_WIFI_PASSWORD);
			g_device->dhcpEn1 = 1;
			g_device->lcd_out = 0;
			g_device->backlight_mode = BY_LIGHTING;		  // по-умолчанию подсветка регулируемая
			g_device->backlight_level = 255;			  // по-умолчанию подсветка максимальная
			strcpy(g_device->tzone, CONFIG_TZONE);		  // по-умолчанию часовой пояс Екатеринбурга
			strcpy(g_device->ntp_server[0], CONFIG_NTP0); // 0
			strcpy(g_device->ntp_server[1], CONFIG_NTP1); // 1
			strcpy(g_device->ntp_server[2], CONFIG_NTP2); // 2
			strcpy(g_device->ntp_server[3], CONFIG_NTP3); // 3

			saveDeviceSettings(g_device);
		}
		else
			ESP_LOGE(TAG, "Device config restored");
	}
	copyDeviceSettings(); // copy in the safe partion
	//
	InitPWM();
	//SPI init for the vs1053 and lcd if spi.
	VS1053_spi_init();
	//
	g_device->audio_input_num = COMPUTER;
	//ESP_LOGI(TAG, "CHECK GPIO_MODE: %d", get_gpio_mode());
	//audio input number COMPUTER, RADIO, BLUETOOTH
	ESP_LOGI(TAG, "audio input number %d\nOne of COMPUTER = 1, RADIO, BLUETOOTH", g_device->audio_input_num);

	// init softwares
	telnetinit();
	websocketinit();
	// log level
	setLogLevel(g_device->trace_level);
	//time display
	uint8_t ddmm;
	get_ddmm(&ddmm);
	setDdmm(ddmm ? 1 : 0);

	init_hardware();
	ESP_LOGI(TAG, "Hardware init done...");
	// lcd init
	uint8_t rt;
	get_lcd_rotat(&rt);
	ESP_LOGI(TAG, "LCD Rotat %d", rt);
	//lcd rotation
	setRotat(rt);
	lcd_init();

	//Initialize the SPI RAM chip communications and see if it actually retains some bytes. If it
	//doesn't, warn user.
	if (bigRam)
	{
		setSPIRAMSIZE(420 * 1024); // room in psram
		ESP_LOGI(TAG, "Set Song buffer to 420k");
	}
	else
	{
		setSPIRAMSIZE(50 * 1024); // more free heap
		ESP_LOGI(TAG, "Set Song buffer to 50k");
	}

	if (!spiRamFifoInit())
	{
		ESP_LOGE(TAG, "SPI RAM chip fail!");
		esp_restart();
	}

	//uart speed
	uspeed = g_device->uartspeed;
	uspeed = checkUart(uspeed);
	uart_set_baudrate(UART_NUM_0, uspeed);
	ESP_LOGI(TAG, "Set baudrate at %d", uspeed);
	if (g_device->uartspeed != uspeed)
	{
		g_device->uartspeed = uspeed;
		saveDeviceSettings(g_device);
	}
	uartBtInit();

	// volume
	setIvol(g_device->vol);
	ESP_LOGI(TAG, "Volume set to %d", g_device->vol);

	// queue for events of the sleep / wake and Ms timers
	event_queue = xQueueCreate(30, sizeof(queue_event_t));
	//
	xTaskCreatePinnedToCore(timerTask, "timerTask", 2100, NULL, PRIO_TIMER, &pxCreatedTask, CPU_TIMER);
	ESP_LOGI(TAG, "%s task: %x", "t0", (unsigned int)pxCreatedTask);

	lcd_welcome("", "ЗАПУСК СЕТИ");
	//-----------------------------
	// start the network
	//-----------------------------
	/* init wifi & network*/
	start_wifi();
	start_network();

	//-----------------------------------------------------
	//init softwares
	//-----------------------------------------------------

	clientInit();
	//initialize mDNS service
	err = mdns_init();
	if (err)
		ESP_LOGE(TAG, "mDNS Init failed: %d", err);
	else
		ESP_LOGE(TAG, "mDNS Init ok");

	//set hostname and instance name
	if ((strlen(g_device->hostname) == 0) || (strlen(g_device->hostname) > HOSTLEN))
	{
		strcpy(g_device->hostname, "ESP32Radiola");
	}
	ESP_LOGE(TAG, "mDNS Hostname: %s", g_device->hostname);
	err = mdns_hostname_set(g_device->hostname);
	if (err)
		ESP_LOGE(TAG, "Hostname Init failed: %d", err);

	ESP_ERROR_CHECK(mdns_instance_name_set(g_device->hostname));
	ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
	ESP_ERROR_CHECK(mdns_service_add(NULL, "_telnet", "_tcp", 23, NULL, 0));

	// init player config
	player_config = (player_t *)calloc(1, sizeof(player_t));
	player_config->command = CMD_NONE;
	player_config->decoder_status = UNINITIALIZED;
	player_config->decoder_command = CMD_NONE;
	player_config->buffer_pref = BUF_PREF_SAFE;
	player_config->media_stream = calloc(1, sizeof(media_stream_t));

	audio_player_init(player_config);

	// LCD Display infos
	lcd_welcome("", "");
	lcd_welcome("", "ЗАПУСК ЗАДАЧ");
	vTaskDelay(100);
	ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());

	//start tasks of ESP32Radiola
	xTaskCreatePinnedToCore(uartInterfaceTask, "uartInterfaceTask", 2400, NULL, PRIO_UART, &pxCreatedTask, CPU_UART);
	ESP_LOGI(TAG, "%s task: %x", "uartInterfaceTask", (unsigned int)pxCreatedTask);
	vTaskDelay(1);
	xTaskCreatePinnedToCore(clientTask, "clientTask", 3000, NULL, PRIO_CLIENT, &pxCreatedTask, CPU_CLIENT);
	ESP_LOGI(TAG, "%s task: %x", "clientTask", (unsigned int)pxCreatedTask);
	vTaskDelay(1);
	xTaskCreatePinnedToCore(serversTask, "serversTask", 3100, NULL, PRIO_SERVER, &pxCreatedTask, CPU_SERVER);
	ESP_LOGI(TAG, "%s task: %x", "serversTask", (unsigned int)pxCreatedTask);
	vTaskDelay(1);
	xTaskCreatePinnedToCore(task_addon, "task_addon", 2200, NULL, PRIO_ADDON, &pxCreatedTask, CPU_ADDON);
	ESP_LOGI(TAG, "%s task: %x", "task_addon", (unsigned int)pxCreatedTask);

	/*	if (RDA5807M_detection())
	{
		xTaskCreatePinnedToCore(rda5807Task, "rda5807Task", 2500, NULL, 3, &pxCreatedTask,1);  //
		ESP_LOGI(TAG, "%s task: %x","rda5807Task",(unsigned int)pxCreatedTask);
	}
*/
	vTaskDelay(60); // wait tasks init
	beep(30);		// бип...
	ESP_LOGI(TAG, " Init Done");

	setIvol(g_device->vol);
	kprintf("READY. Type help for a list of commands\n");

	//autostart
	autoPlay();
	// All done.
}
