#include "flash_manager_cli.h"


#include <furi-hal-delay.h>
#include <irda.h>
#include <app-template.h>
#include <cli/cli.h>
#include <cmsis_os2.h>
#include <irda_worker.h>
#include <furi.h>
#include <furi-hal-irda.h>
#include <sstream>
#include <string>
#include <m-string.h>
#include <irda_transmit.h>
#include <sys/types.h>


static void flash_manager_cli_i2c_find(Cli* cli, string_t args, void* context) { 
    printf("Hello world!.\r\n"); 

    furi_hal_i2c_acquire(&furi_hal_i2c_handle_external);
    uint8_t test = 0;
    printf("Scanning external i2c on PC0(SCL)/PC1(SDA)\r\n"
           "Clock: 100khz, 7bit address\r\n"
           "!!! Invasive mode (tx to device) !!!\r\n\r\n");
    printf("  | 0 1 2 3 4 5 6 7 8 9 A B C D E F\r\n");
    printf("--+--------------------------------\r\n");
    for(uint8_t row = 0; row < 0x8; row++) {
        printf("%x | ", row);
        for(uint8_t column = 0; column <= 0xF; column++) {
            bool ret =
                furi_hal_i2c_rx(&furi_hal_i2c_handle_external, (row << 4) + column, &test, 1, 2);
            printf("%c ", ret ? '#' : '-');
        }
        printf("\r\n");
    }
    furi_hal_i2c_release(&furi_hal_i2c_handle_external);
}


extern "C" void flash_manager_cli_init() {
    Cli* cli = (Cli*)furi_record_open("cli");
    cli_add_command(cli, "flash_i2c_find", CliCommandFlagDefault, flash_manager_cli_i2c_find, NULL);
    furi_record_close("cli");
}