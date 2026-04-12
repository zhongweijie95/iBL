/*
    \file  gd32g5x3_init.h
*/
/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#ifndef GD32G5X3_INIT_H
#define GD32G5X3_INIT_H

#include "gd32g5x3.h"

#ifdef __cplusplus
extern "C" {
 #endif

/* External Includes*/
/* user code [External Includes] begin */
#include "global_include.h"
/* user code [External Includes] end */

/* Private Type Definitions */
/* user code [Type Definitions] begin */

/* user code [Type Definitions] end */

/* Shared Macros */
#define CAN_RES_TRIG_B9_PIN GPIO_PIN_9
#define CAN_RES_TRIG_B9_GPIO_PORT GPIOB
/* user code [Shared Macros] begin */

/* user code [Shared Macros] end */

/* External Constants  */
/* user code [External Constants] begin */

/* user code [External Constants] end */

/* External Variables */
/* user code [External Variables] begin */

/* user code [External Variables] end */

void msd_system_init(void);
void msd_clock_init(void);
void msd_gpio_init(void);
void msd_gpio_deinit(void);
void msd_can0_init(void);
void msd_can0_deinit(void);
void msd_fwdgt_init(void);
void msd_pmu_init(void);
void msd_pmu_deinit(void);

/* user code [Public Functions] begin */

/* user code [Public Functions] end */

#ifdef __cplusplus
}
 #endif

#endif /*GD32G5X3_INIT_H */
