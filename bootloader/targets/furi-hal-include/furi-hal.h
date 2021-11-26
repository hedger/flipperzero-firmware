#pragma once

#include <furi-hal-i2c.h>
#include <furi-hal-light.h>
#include <furi-hal-resources.h>
#include <furi-hal-spi.h>
#include <furi-hal-version.h>

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
