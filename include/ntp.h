//////////////////////////////////////////////////
// Simple NTP client for ESP32-Radiola.
// Copyright 2019 SinglWolf (https://serverdoma.ru)
// See license.txt for license terms.
//////////////////////////////////////////////////

#ifndef __NTP_H__
#define __NTP_H__
#include <time.h>
#include <sys/time.h>
#include "lwip/apps/sntp.h"

#define _NTP0 "0.ru.pool.ntp.org"
#define _NTP1 "1.ru.pool.ntp.org"
#define _NTP2 "2.ru.pool.ntp.org"
#define _NTP3 "3.ru.pool.ntp.org"

void ntp_print_time();
void initialize_sntp(void);

#endif