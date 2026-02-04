#pragma once

#include <stddef.h>

#include "stdint.h"
#include "at32f403a_407_wk_config.h"
#include "wk_debug.h"
#include "wk_gpio.h"
#include "wk_system.h"

#include "func_ret_code.h"
#include "at32wk_bkp_struct.h"
#include "can_handler.h"
#include "../nvm/at32wk_nvm_address.h"

typedef union reset_flag_t
{
    uint8_t reg;
    struct
    {
        uint8_t low_power_reset  : 1; /* [0] */
        uint8_t window_wdt_reset : 1; /* [1] */
        uint8_t wdt_reset        : 1; /* [2] */
        uint8_t software_reset   : 1; /* [3] */
        uint8_t power_on_reset   : 1; /* [4] */
        uint8_t nrst_reset       : 1; /* [5] */
        uint8_t                  : 2; /* [6:7] */
    } bit;
} reset_flag_t;

extern reset_flag_t reset_flag;
extern bkp_struct_t bkp_struct;

void app_main(void);

FuncRetCode OnFWUpdateGetFileSize(uint32_t size);
FuncRetCode OnFWUpdateReceiveBlock(uint32_t offset, uint8_t* data, uint32_t len);
FuncRetCode OnFWUpdateFinished(void);
FuncRetCode OnRestartNode(void);

// 31(dec) -> 0x31(hex), only <= 2 digits supported
uint8_t dec_to_hex_byte(const uint8_t dec);

uint32_t GetUptimeSeconds(void);

uint32_t GetCurrentTickCount(void);

uint32_t GetSerialNumber(void);

uint32_t GetFirmwareSizeBytes(void);

uint64_t GetFirmwareCRC64(void);

uint8_t get_crc8(const uint8_t* data, const size_t len);

int YEAR(void);
int MONTH(void);

int DAY(void);
int HOUR(void);
int MINUTE(void);

extern volatile uint32_t uwTick;
extern volatile uint8_t timer_1ms;
extern volatile uint8_t timer_10khz;