/* Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru) */
#include "esp_system.h"

//#define CACHE_FLASH __attribute__((section(".irom0.rodata")))

struct servFile
{
	const char name[32];
	const char type[16];
	uint16_t size;
	const char* content;
	struct servFile *next;
};

//ICACHE_STORE_ATTR ICACHE_RODATA_ATTR
#define ICACHE_STORE_TYPEDEF_ATTR __attribute__((aligned(4),packed))
#define ICACHE_STORE_ATTR __attribute__((aligned(4)))
#define ICACHE_RAM_ATTR __attribute__((section(".iram0.text")))

#include "../../webpage/.index"
#include "../../webpage/.style"
#include "../../webpage/.script"
#include "../../webpage/.tabbis"
#include "../../webpage/.logo"
#include "../../webpage/.favicon"

const struct servFile faviconFile = {
	"/favicon.png",
	"image/png",
	sizeof(favicon_png),
	(const char*)favicon_png,
	(struct servFile*)NULL
};
const struct servFile logoFile = {
	"/logo.png",
	"image/png",
	sizeof(logo_png),
	(const char*)logo_png,
	(struct servFile*)&faviconFile
};

const struct servFile scriptFile = {
	"/script.js",
	"text/javascript",
	sizeof(script_js),
	(const char*)script_js,
	(struct servFile*)&logoFile
};
const struct servFile scriptTabbis = {
	"/tabbis.js",
	"text/javascript",
	sizeof(tabbis_js),
	(const char*)tabbis_js,
	(struct servFile*)&scriptFile
};

const struct servFile styleFile = {
	"/style.css",
	"text/css",
	sizeof(style_css),
	(const char*)style_css,
	(struct servFile*)&scriptTabbis
};

const struct servFile indexFile = {
	"/",
	"text/html",
	sizeof(index_html),
	(const char*)index_html,
	(struct servFile*)&styleFile
};
