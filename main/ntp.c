//////////////////////////////////////////////////
// Simple NTP client for ESP32-Radiola.
// Copyright 2019 SinglWolf (https://serverdoma.ru)
// See license.txt for license terms.
//////////////////////////////////////////////////
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define TAG "NTP"
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "ntp.h"
#include "interface.h"
#include "eeprom.h"

// print  date time in ISO-8601 local time format
void ntp_print_time()
{
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);

	char msg[30];

	strftime(msg, 48, "%Y-%m-%dT%H:%M:%S", &timeinfo);
	//	ISO-8601 local time   https://www.w3.org/TR/NOTE-datetime
	//  YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
	kprintf("##SYS.DATE#: %s\n", msg);
}

void initialize_sntp(void)
{
	ESP_LOGI(TAG, "Initializing SNTP");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, g_device->ntp_server[0]);
	sntp_setservername(1, g_device->ntp_server[1]);
	sntp_setservername(2, g_device->ntp_server[2]);
	sntp_setservername(3, g_device->ntp_server[3]);
	sntp_init();
}
