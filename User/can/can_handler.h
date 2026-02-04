#pragma once

#include "global_include.h"
#include "kfifo_s.h"
#include "can_message.h"
#include "func_ret_code.h"

void InitCAN(can_type *hcan);

void OnCANIRQ(can_type *hcan);

uint8_t PeekCANRxFIFO(void);

uint8_t GetCANRxFIFO(CANMessage* dst);

FuncRetCode TransmitCANMessage(can_type *hcan, CANMessage* msg);
