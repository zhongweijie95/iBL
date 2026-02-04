//
// Created by vip99 on 2/3/2026.
//

#ifndef AT32F403A_BL_BKP_H
#define AT32F403A_BL_BKP_H

#include "global_include.h"
#include "at32wk_bkp_struct.h"
#include <stdbool.h>

void initialize_bkp(bkp_struct_t* dst);
bool read_bpr(bkp_struct_t* dst);
void write_bpr(bkp_struct_t* src);

#endif //AT32F403A_BL_BKP_H