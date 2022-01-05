#pragma once

#include <furi_hal_i2c.h>
#include <furi_hal_light.h>
#include <furi_hal_resources.h>
#include <furi_hal_spi.h>
#include <furi_hal_version.h>

#ifndef furi_assert
#define furi_assert(value) (void)(value)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_init();

void delay(float milliseconds);

void delay_us(float microseconds);

#ifdef __cplusplus
}
#endif
