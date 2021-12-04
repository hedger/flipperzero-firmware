#include "spi_impl.h"

#ifndef FLASMMGR_SPI_BITBANG

#include <furi.h>
#include <furi-hal.h>

#include <furi-hal-spi.h>
#include <furi-hal-spi-config.h>

#define SPI_TIMEOUT 500

#define TAG "SPIImpl"

#pragma region SPI CONFIG
//static FuriHalSpiBusHandle* const p_spi_bus = &furi_hal_spi_bus_handle_external;

//!!! #define USE_SPI_LOG

static const LL_SPI_InitTypeDef flash_manager_spi_preset_external_safe = {
    .Mode = LL_SPI_MODE_MASTER,
    .TransferDirection = LL_SPI_FULL_DUPLEX,
    .DataWidth = LL_SPI_DATAWIDTH_8BIT,
    .ClockPolarity = LL_SPI_POLARITY_LOW,
    .ClockPhase = LL_SPI_PHASE_1EDGE,
    .NSS = LL_SPI_NSS_SOFT,
#ifdef USE_SPI_LOG
    .BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16, // DIV256,
#else
    .BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16,
#endif
    .BitOrder = LL_SPI_MSB_FIRST,
    .CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE,
    .CRCPoly = 7,
};

// TODO: remove when better HAL api arrives, pure c+p of furi_hal_spi_bus_r_handle_event_callback
inline static void flash_manager_spi_handle_event_callback(
    FuriHalSpiBusHandle* handle,
    FuriHalSpiBusHandleEvent event,
    const LL_SPI_InitTypeDef* preset) {
    if(event == FuriHalSpiBusHandleEventInit) {
        hal_gpio_write(handle->cs, true);
        hal_gpio_init(handle->cs, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    } else if(event == FuriHalSpiBusHandleEventDeinit) {
        hal_gpio_write(handle->cs, true);
        hal_gpio_init(handle->cs, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    } else if(event == FuriHalSpiBusHandleEventActivate) {
        LL_SPI_Init(handle->bus->spi, (LL_SPI_InitTypeDef*)preset);
        LL_SPI_SetRxFIFOThreshold(handle->bus->spi, LL_SPI_RX_FIFO_TH_QUARTER);
        LL_SPI_Enable(handle->bus->spi);

        hal_gpio_init_ex(
            handle->miso,
            GpioModeAltFunctionPushPull,
            GpioPullNo,
            GpioSpeedVeryHigh,
            GpioAltFn5SPI1);
        hal_gpio_init_ex(
            handle->mosi,
            GpioModeAltFunctionPushPull,
            GpioPullNo,
            GpioSpeedVeryHigh,
            GpioAltFn5SPI1);
        hal_gpio_init_ex(
            handle->sck,
            GpioModeAltFunctionPushPull,
            GpioPullNo,
            GpioSpeedVeryHigh,
            GpioAltFn5SPI1);

        hal_gpio_write(handle->cs, false);
    } else if(event == FuriHalSpiBusHandleEventDeactivate) {
        hal_gpio_write(handle->cs, true);

        hal_gpio_init(handle->miso, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
        hal_gpio_init(handle->mosi, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
        hal_gpio_init(handle->sck, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

        LL_SPI_Disable(handle->bus->spi);
    }
}

static void flash_manager_spi_handle_bus_event_callback(
    FuriHalSpiBusHandle* handle,
    FuriHalSpiBusHandleEvent event) {
    flash_manager_spi_handle_event_callback(
        handle, event, &flash_manager_spi_preset_external_safe);
}

FuriHalSpiBusHandle furi_hal_spi_bus_handle_external_flash = {
    .bus = &furi_hal_spi_bus_r,
    .callback = flash_manager_spi_handle_bus_event_callback,
    .miso = &gpio_ext_pa6,
    .mosi = &gpio_ext_pa7,
    .sck = &gpio_ext_pb3,
    .cs = &gpio_ext_pa4,
};

static FuriHalSpiBusHandle* const p_spi_bus = &furi_hal_spi_bus_handle_external_flash;

///////////////////////////////////////////

bool spi_wrapper_init() {
    furi_hal_spi_bus_handle_init(p_spi_bus);
    return true;
}

void spi_wrapper_deinit() {
    furi_hal_spi_bus_handle_deinit(p_spi_bus);
}

void spi_wrapper_acquire_bus() {
    furi_hal_spi_acquire(p_spi_bus);
}

void spi_wrapper_release_bus() {
    furi_hal_spi_release(p_spi_bus);
}

#ifdef USE_SPI_LOG
void log_buffer_as_hex(uint8_t* inptr, const int size) {
    char *buf_str = (char*)malloc(3 * size), *buf_ptr = buf_str;
    if(buf_str) {
        for(int i = 0; i < size; i++) {
            buf_ptr += sprintf(buf_ptr, i < size - 1 ? "%02X:" : "%02X", inptr[i]);
        }
        FURI_LOG_I(TAG, "%s", buf_str);
        free(buf_str);
    }
}
#endif

bool spi_wrapper_write_read(
    uint8_t opCode,
    uint8_t* write_data,
    int write_len,
    uint8_t* read_data,
    int read_len) {
    hal_gpio_write(p_spi_bus->cs, true);
    bool result = false;

#ifdef USE_SPI_LOG
    uint8_t* tx_buffer = malloc(1 + write_len);
    tx_buffer[0] = opCode;
    if(write_len != 0) {
        memcpy(&tx_buffer[1], write_data, write_len);
    }
    FURI_LOG_I(TAG, "SPI TX");
    log_buffer_as_hex(tx_buffer, 1 + write_len);
    free(tx_buffer);
#endif

    hal_gpio_write(p_spi_bus->cs, false);
    for(;;) {
        if(!furi_hal_spi_bus_tx(p_spi_bus, &opCode, 1, SPI_TIMEOUT)) break;
        if(write_data != NULL && write_len != 0 &&
           !furi_hal_spi_bus_tx(p_spi_bus, write_data, write_len, SPI_TIMEOUT))
            break;

        if(read_data != NULL && read_len != 0 &&
           !furi_hal_spi_bus_rx(p_spi_bus, read_data, read_len, SPI_TIMEOUT))
            break;
#ifdef USE_SPI_LOG
        if(read_data) {
            FURI_LOG_I(TAG, "SPI RX");
            log_buffer_as_hex(read_data, read_len);
        }
#endif
        result = true;
        break;
    }
    hal_gpio_write(p_spi_bus->cs, true);

    return result;
}

#endif // FLASMMGR_SPI_BITBANG