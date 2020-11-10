/* Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru) */
#ifndef __SERV_FS_H__
#define __SERV_FS_H__
#include "esp_system.h"

//#define CACHE_FLASH __attribute__((section(".irom0.rodata")))

struct servFile
{
	const char name[32];
	const char type[16];
	uint16_t size;
	const char *content;
	struct servFile *next;
};

//ICACHE_STORE_ATTR ICACHE_RODATA_ATTR
#define ICACHE_STORE_TYPEDEF_ATTR __attribute__((aligned(4), packed))
#define ICACHE_STORE_ATTR __attribute__((aligned(4)))
#define ICACHE_RAM_ATTR __attribute__((section(".iram0.text")))

#ifdef CONFIG_RELEASE
#include "index_release.h"
#include "script_release.h"
#include "style_release.h"
#include "icons_release.h"
#else
#include "index_debug.h"
#include "script_debug.h"
#include "style_debug.h"
#include "icons_debug.h"
#endif

#include "favicon.h"
#include "logo.h"
#include "tabbis.h"

const struct servFile faviconFile = {
	"/favicon.png",
	"image/png",
	sizeof(favicon),
	(const char *)favicon,
	(struct servFile *)NULL};
const struct servFile logoFile = {
	"/logo.png",
	"image/png",
	sizeof(logo),
	(const char *)logo,
	(struct servFile *)&faviconFile};

const struct servFile scriptFile = {
	"/script.js",
	"text/javascript",
	sizeof(script),
	(const char *)script,
	(struct servFile *)&logoFile};
const struct servFile scriptTabbis = {
	"/tabbis.js",
	"text/javascript",
	sizeof(tabbis),
	(const char *)tabbis,
	(struct servFile *)&scriptFile};

const struct servFile styleFile = {
	"/style.css",
	"text/css",
	sizeof(style),
	(const char *)style,
	(struct servFile *)&scriptTabbis};

const struct servFile styleIcons = {
	"/icons.css",
	"text/css",
	sizeof(icons),
	(const char *)icons,
	(struct servFile *)&styleFile};

const struct servFile indexFile = {
	"/",
	"text/html",
	sizeof(index_p),
	(const char *)index_p,
	(struct servFile *)&styleIcons};
#endif