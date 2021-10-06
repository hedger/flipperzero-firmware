#include "test-plugin_cli.h"

#include <furi.h>
#include <furi-hal.h>

void test_plugin_cli_init() {
    Cli* cli = furi_record_open("cli");

    cli_add_command(cli, "hello", CliCommandFlagDefault, test_plugin_cli_command_hello, NULL);

    furi_record_close("cli");
}

void test_plugin_cli_command_hello(Cli* cli, string_t args, void* context) {
    // uint32_t frequency = 433920000;
    printf("Hello world");
    // cli_print_usage("hello", "<any data>", string_get_cstr(args));
}