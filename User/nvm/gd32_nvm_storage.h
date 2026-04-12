#pragma once

#include "global_include.h"

uint32_t ceil_to_n_divisible(uint32_t num, uint32_t n);

uint32_t GetPage(const uint32_t Addr);

FuncRetCode Erase(uint32_t addr, size_t size);

FuncRetCode Write_NoErase(uint32_t addr, const uint8_t* buffer, size_t size);

FuncRetCode Read(uint32_t addr, uint8_t* buffer, const size_t size);