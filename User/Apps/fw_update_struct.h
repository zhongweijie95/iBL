#pragma once

#include <stdint.h>

typedef struct fw_update_struct_t
{
    uint8_t file_server_node_id;
    uint8_t path[7];
    uint8_t transfer_id;
    uint8_t error;
    uint32_t file_full_size;
    uint32_t offset;
    uint32_t last_requested_offset;
    uint32_t last_requested_tick;
    uint32_t last_response_tick;
}fw_update_struct_t;