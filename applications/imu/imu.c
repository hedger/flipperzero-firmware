/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "lis331dlh_reg.h"

#include <furi.h>
#include <furi-hal.h>
#include "imu.h"

#define SENSOR_BUS EXT_I2C


/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME          5 //ms
/* Private variables ---------------------------------------------------------*/
static int16_t data_raw_acceleration[3];
static float acceleration_mg[3];
static uint8_t whoamI;
static uint8_t tx_buffer[1000];

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);

/* Main Example --------------------------------------------------------------*/
void lis331dlh_read_data_polling(void)
{
  /* Initialize mems driver interface */
  stmdev_ctx_t dev_ctx;
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = SENSOR_BUS;
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  lis331dlh_device_id_get(&dev_ctx, &whoamI);
  
  if (whoamI != LIS331DLH_ID) {
      /* manage here device not found */
      printf("%x", whoamI);
      printf("\nDevice not found");
  } else {
    /* manage here device found */
      printf("%x", whoamI);
      printf("\nDevice found!");
  }

  /* Enable Block Data Update */
  lis331dlh_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set full scale */
  lis331dlh_full_scale_set(&dev_ctx, LIS331DLH_2g);
  
  /* Configure filtering chain */
  /* Accelerometer - High Pass / Slope path */
  lis331dlh_hp_path_set(&dev_ctx, LIS331DLH_HP_DISABLE);
  // lis331dlh_hp_path_set(&dev_ctx, LIS331DLH_HP_ON_OUT);
  // lis331dlh_hp_reset_get(&dev_ctx);

  /* Set Output Data Rate */
  lis331dlh_data_rate_set(&dev_ctx, LIS331DLH_ODR_100Hz);

  /* Read samples */

    /* Check if new value is available in status_reg.zyxda */
    lis331dlh_reg_t reg;
    lis331dlh_status_reg_get(&dev_ctx, &reg.status_reg);

      /* Read acceleration data */
      memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
      lis331dlh_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
      acceleration_mg[0] =
        lis331dlh_from_fs2_to_mg(data_raw_acceleration[0]);
      acceleration_mg[1] =
        lis331dlh_from_fs2_to_mg(data_raw_acceleration[1]);
      acceleration_mg[2] =
        lis331dlh_from_fs2_to_mg(data_raw_acceleration[2]);
      sprintf((char *)tx_buffer,
              "Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n",
              acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
}

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
  reg |= 0x80;
  uint8_t buffer[2] = {reg, *bufp};
  furi_hal_i2c_tx(EXT_I2C, LIS331DLH_I2C_ADD_L, buffer, len, 1000);

  return 0;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
  reg |= 0x80;
  furi_hal_i2c_trx(EXT_I2C, LIS331DLH_I2C_ADD_L, &reg, 1, bufp, len, 1000);
  return 0;
}

static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
    printf((char *)tx_buffer);
}

static void platform_delay(uint32_t ms)
{
    osDelay(ms);
}

// CLI func init

void imu_cli_init() {
    Cli* cli = furi_record_open("cli");
    cli_add_command(cli, "imu", CliCommandFlagDefault, imu_cli_command_imu, NULL);
    furi_record_close("cli");
}

void imu_cli_command_imu(Cli* cli, string_t args, void* context) {
    printf("IMU start\n");
    lis331dlh_read_data_polling();
}