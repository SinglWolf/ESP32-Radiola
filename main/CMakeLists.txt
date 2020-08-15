# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.c)

idf_component_register(SRCS ${app_sources}

INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include"

REQUIRES "nvs_flash app_update mdns esp_adc_cal"
)
