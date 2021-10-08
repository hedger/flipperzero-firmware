/*
 ******************************************************************************
 * @file    orientation_6d.c
 * @author  Sensors Software Solution Team
 * @brief   This file show the simplest way to detect 6D orientation from sensor.
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3 + STEVAL-MKI206V1
 * - NUCLEO_F411RE + STEVAL-MKI206V1
 * - DISCOVERY_SPC584B + STEVAL-MKI206V1
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * DISCOVERY_SPC584B  - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */

//#define STEVAL_MKI109V3  /* little endian */
//#define NUCLEO_F411RE    /* little endian */
//#define SPC584B_DIS      /* big endian */

/* ATTENTION: By default the driver is little endian. If you need switch
 *            to big endian please see "Endianness definitions" in the
 *            header file of the driver (_reg.h).
 */


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "lis331dlh_reg.h"

#include <furi.h>
#include <furi-hal.h>
#include "imu.h"

// #include <stm32wbxx_hal_i2c.h>

#define SENSOR_BUS EXT_I2C


/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME          5 //ms
/* Private variables ---------------------------------------------------------*/
static uint8_t whoamI;
static uint8_t tx_buffer[1000];

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);

/* Main Example --------------------------------------------------------------*/
void lis331dlh_orientation_6D(void)
{
  /*
   * Initialize mems driver interface */
  stmdev_ctx_t dev_ctx;
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = SENSOR_BUS;

//   furi_hal_power_enable_otg();

  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  lis331dlh_device_id_get(&dev_ctx, &whoamI);

    printf("%x", whoamI);

  if (whoamI != LIS331DLH_ID) {
    while (1) {
      /* manage here device not found */
      printf("\nDevice not found");
      break;
    }
  } else {
        printf("\nDevice found!");
        tx_com(tx_buffer, strlen((char const *)tx_buffer));
  }
  // /* Set full scale */
  // lis331dlh_full_scale_set(&dev_ctx, LIS331DLH_2g);
  // /* Disable HP filter */
  // lis331dlh_hp_path_set(&dev_ctx, LIS331DLH_HP_DISABLE);
  // /* Set no duration */
  // lis331dlh_int1_dur_set(&dev_ctx, 0);
  // /* Set 6D position detection */
  // lis331dlh_int1_6d_mode_set(&dev_ctx, LIS331DLH_6D_INT1_POSITION);
  // /* Apply 6D Orientation axis threshold */
  // lis331dlh_int1_treshold_set(&dev_ctx, 33);
  // /* Set Output Data Rate */
  // lis331dlh_data_rate_set(&dev_ctx, LIS331DLH_ODR_100Hz);

  /* Wait Events */
  // while (1) {
  //   lis331dlh_reg_t all_source;
  //   lis331dlh_int1_src_get(&dev_ctx, &all_source.int1_src);

  //   /* Check 6D Orientation */
  //   switch (all_source.byte & 0x3f) {
  //     case 0x01:
  //       sprintf((char *)tx_buffer, "6D Or. position XL\r\n");
  //       tx_com(tx_buffer, strlen((char const *)tx_buffer));
  //       break;

  //     case 0x02:
  //       sprintf((char *)tx_buffer, "6D Or. position XH\r\n");
  //       tx_com(tx_buffer, strlen((char const *)tx_buffer));
  //       break;

  //     case 0x04:
  //       sprintf((char *)tx_buffer, "6D Or. position YL\r\n");
  //       tx_com(tx_buffer, strlen((char const *)tx_buffer));
  //       break;

  //     case 0x08:
  //       sprintf((char *)tx_buffer, "6D Or. position YH\r\n");
  //       tx_com(tx_buffer, strlen((char const *)tx_buffer));
  //       break;

  //     case 0x10:
  //       sprintf((char *)tx_buffer, "6D Or. position ZL\r\n");
  //       tx_com(tx_buffer, strlen((char const *)tx_buffer));
  //       break;

  //     case 0x20:
  //       sprintf((char *)tx_buffer, "6D Or. position ZH\r\n");
  //       tx_com(tx_buffer, strlen((char const *)tx_buffer));
  //       break;

  //     default:
  //       break;
  //   }
  // }
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */


static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
  reg |= 0x80;
  uint8_t buffer[2] = {reg, *bufp};
  furi_hal_i2c_tx(EXT_I2C, LIS331DLH_I2C_ADD_L, buffer, len, 1000);

  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
  reg |= 0x80;
  uint8_t buffer[2] = {reg, *bufp};
  furi_hal_i2c_rx(EXT_I2C, LIS331DLH_I2C_ADD_L, buffer, len, 1000);

  return 0;
}



/*
 * @brief  Send buffer to console (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
    printf((char *)tx_buffer);
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
    osDelay(ms);
}

void imu_cli_init() {
    Cli* cli = furi_record_open("cli");
    cli_add_command(cli, "imu", CliCommandFlagDefault, imu_cli_command_imu, NULL);
    furi_record_close("cli");
}

void imu_cli_command_imu(Cli* cli, string_t args, void* context) {
    printf("IMU start\n");
    lis331dlh_orientation_6D();
}