#include "flash_manager_cli.h"

#include <furi_hal_delay.h>
#include <irda.h>
#include <app_template.h>
#include <cli/cli.h>
#include <cmsis_os2.h>
#include <irda_worker.h>
#include <furi.h>
#include <sstream>
#include <string>
#include <m-string.h>
#include <sys/types.h>

static void flash_manager_cli_i2c_find(Cli* cli, string_t args, void* context) {
    printf("Hello world!.\r\n");

    FuriHalI2cBusHandle* p_bus = &furi_hal_i2c_handle_external;
    furi_hal_i2c_acquire(p_bus);

    uint8_t tx_buf[] = {0x10};
    uint8_t rx_buf[0x1] = {0xCD};

    bool res =
        furi_hal_i2c_trx(p_bus, 0x50, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf), 1000);
    printf("result = %d, buf = %x\r\n", res, rx_buf[0]);


    furi_hal_i2c_release(p_bus);
}

extern "C" void flash_manager_on_system_start() {
    Cli* cli = (Cli*)furi_record_open("cli");
    cli_add_command(
        cli, "flash_i2c_find", CliCommandFlagDefault, flash_manager_cli_i2c_find, NULL);
    furi_record_close("cli");
}