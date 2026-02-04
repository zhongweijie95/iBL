#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct CANMessage
{
    uint32_t cob_id;
    uint8_t is_ext;
    uint8_t is_rtr;
    uint8_t len;
    uint8_t data[8];
} CANMessage;