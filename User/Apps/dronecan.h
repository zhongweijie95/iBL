#pragma once

#include "global_include.h"
#include "canard.h"
#include "fw_update_struct.h"
#include "can_handler.h"

extern struct uavcan_protocol_NodeStatus node_status;
// extern uint8_t desired_node_id;
extern fw_update_struct_t fw_update;

void InitDroneCAN(void);

void ProcessDroneCAN_RT(void);

void ProcessDroneCAN_1KHz(void);

void ProcessDroneCAN_10KHz(void);