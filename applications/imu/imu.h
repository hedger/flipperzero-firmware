#pragma once

#include <cli/cli.h>

void imu_cli_init();

void imu_cli_command_imu(Cli* cli, string_t args, void* context);