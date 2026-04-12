#pragma once

#include "global_include.h"
#include "kfifo_s.h"
#include "can_message.h"
#include "func_ret_code.h"

void InitCAN(uint32_t hcan);

void OnCANIRQ(uint32_t hcan);

uint8_t PeekCANRxFIFO(void);

uint8_t GetCANRxFIFO(CANMessage* dst);

FuncRetCode TransmitCANMessage(uint32_t hcan, CANMessage* msg);
