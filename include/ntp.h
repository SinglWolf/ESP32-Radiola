//////////////////////////////////////////////////
// Simple NTP client for ESP8266.
// Copyright 2016 jp cocatrix (KaraWin)
// jp@karawin.fr
// Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
// See license.txt for license terms.
//////////////////////////////////////////////////

#ifndef __NTP_H__
#define __NTP_H__
#include <time.h>
#include <sys/time.h>
#include "lwip/apps/sntp.h"

// typedef struct {
// 	uint8_t options;
// 	uint8_t stratum;
// 	uint8_t poll;
// 	uint8_t precision;
// 	uint32_t root_delay;
// 	uint32_t root_disp;
// 	uint32_t ref_id;
// 	uint8_t ref_time[8];
// 	uint8_t orig_time[8];
// 	uint8_t recv_time[8];
// 	uint8_t trans_time[8];
// } ntp_t;

#define _NTP0 "0.ru.pool.ntp.org"
#define _NTP1 "1.ru.pool.ntp.org"
#define _NTP2 "2.ru.pool.ntp.org"
#define _NTP3 "3.ru.pool.ntp.org"

//void ntpTask(void *pvParams);

// print locale date time in ISO-8601 local time
bool ntp_get_time(struct tm **dt);
void ntp_print_time();
//int8_t  applyTZ(struct tm *time);
void initialize_sntp(void);

#endif