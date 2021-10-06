#pragma once

#include <cli/cli.h>

void test_plugin_cli_init();

void test_plugin_cli_command_hello(Cli* cli, string_t args, void* context);