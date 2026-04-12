/*
    \file  gd32g5x3_init.c
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

#include "gd32g5x3_init.h"
#include "gd32g5x3_it.h"

/* External Includes*/
/* user code [External Includes] begin */

/* user code [External Includes] end */

/* Private Type Definitions */
/* user code [Private Type Definitions] begin */

/* user code [Private Type Definitions] end */

/* Private Macros */
/* user code [Private Macros] begin */

/* user code [Private Macros] end */

/* Private Constants */
/* user code [Private Constants] begin */

/* user code [Private Constants] end */

/* Private Variables */
/* user code [Private Variables] begin */

/* user code [Private Variables] end */

/* Private Function Declaration */
/* user code [Private Function Declaration] begin */
uint8_t SysTick_Config_UNUSED(uint32_t x) { return 0U; }
#define SysTick_Config SysTick_Config_UNUSED
/* user code [Private Function Declaration] end */

/* Extern Variables */
/* user code [Extern Variables] begin */

/* user code [Extern Variables] end */

/*!
    \brief      SYSTEM initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_system_init(void)
{
    /* user code [system_init local 0] begin */

    /* user code [system_init local 0] end */

    rcu_periph_clock_enable(RCU_SYSCFG);
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    systick_clksource_set(SYSTICK_CLKSOURCE_HCLK);
    nvic_exception_irq_enable(MemoryManagement_IRQn, 0, 0);
    nvic_exception_irq_enable(BusFault_IRQn, 0, 0);
    nvic_exception_irq_enable(UsageFault_IRQn, 0, 0);
    nvic_exception_irq_enable(SVCall_IRQn, 0, 0);
    nvic_exception_irq_enable(DebugMonitor_IRQn, 0, 0);
    nvic_exception_irq_enable(PendSV_IRQn, 0, 0);

    /* user code [system_init local 1] begin */

    /* user code [system_init local 1] end */
}

/*!
    \brief      CLOCK initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_clock_init(void)
{
    /* user code [clock_init local 0] begin */

    /* user code [clock_init local 0] end */

    rcu_osci_on(RCU_HXTAL);
    while (rcu_osci_stab_wait(RCU_HXTAL) != SUCCESS);

    rcu_pll_source_config(RCU_PLLSRC_HXTAL);
    rcu_pll_config((1U), (54U), (2U), (2U), (2U));
    rcu_pll_clock_output_enable(RCU_PLLP);
    fmc_wscnt_set(FMC_WAIT_STATE_7);
    rcu_system_clock_source_config(RCU_CKSYSSRC_PLLP);
    rcu_osci_on(RCU_PLL_CK);
    while (rcu_osci_stab_wait(RCU_PLL_CK) != SUCCESS);

    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);
    rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV1);
    rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV1);
    rcu_apb3_clock_config(RCU_APB3_CKAHB_DIV1);

    rcu_can_clock_config(IDX_CAN0, RCU_CANSRC_APB2);

    rcu_osci_on(RCU_IRC32K);
    while (rcu_osci_stab_wait(RCU_IRC32K) != SUCCESS);

    rcu_osci_bypass_mode_disable(RCU_HXTAL);
    rcu_osci_bypass_mode_disable(RCU_LXTAL);
    rcu_hxtal_clock_monitor_disable();
    rcu_lxtal_clock_monitor_disable();

    /* update SystemCoreClock value */
    SystemCoreClockUpdate();

    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        /* capture error */
        while (1);
    }

    /* user code [clock_init local 1] begin */

    /* user code [clock_init local 1] end */
}

/*!
    \brief      GPIO initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_gpio_init(void)
{
    /* user code [gpio_init local 0] begin */

    /* user code [gpio_init local 0] end */

    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_bit_set(CAN_RES_TRIG_B9_GPIO_PORT, CAN_RES_TRIG_B9_PIN);
    gpio_mode_set(CAN_RES_TRIG_B9_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, CAN_RES_TRIG_B9_PIN);
    gpio_output_options_set(CAN_RES_TRIG_B9_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, CAN_RES_TRIG_B9_PIN);

    /* user code [gpio_init local 1] begin */

    /* user code [gpio_init local 1] end */
}

/*!
    \brief      GPIO deinitialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_gpio_deinit(void)
{
    /* user code [gpio_deinit local 0] begin */

    /* user code [gpio_deinit local 0] end */

    rcu_periph_clock_disable(RCU_GPIOB);
    rcu_periph_clock_disable(RCU_GPIOA);

    /* user code [gpio_deinit local 1] begin */

    /* user code [gpio_deinit local 1] end */
}

/*!
    \brief      CAN0 initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_can0_init(void)
{
    /* user code [can0_init local 0] begin */

    /* user code [can0_init local 0] end */

    can_parameter_struct can0_parameter;

    rcu_periph_clock_enable(RCU_CAN0);

    gpio_af_set(GPIOA, GPIO_AF_9, GPIO_PIN_11);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_11);
    gpio_input_filter_set(GPIOA, GPIO_ISPERIOD(0), GPIO_IFTYPE_ASYNC, GPIO_PIN_11);

    gpio_af_set(GPIOA, GPIO_AF_9, GPIO_PIN_12);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_85MHZ, GPIO_PIN_12);
    gpio_input_filter_set(GPIOA, GPIO_ISPERIOD(0), GPIO_IFTYPE_ASYNC, GPIO_PIN_12);

    can_struct_para_init(CAN_INIT_STRUCT, &can0_parameter);

    can0_parameter.internal_counter_source = CAN_TIMER_SOURCE_BIT_CLOCK;
    can0_parameter.mb_tx_order = CAN_TX_HIGH_PRIORITY_MB_FIRST;
    can0_parameter.mb_rx_ide_rtr_type = CAN_IDE_RTR_COMPARED;
    can0_parameter.mb_remote_frame = CAN_GEN_REMOTE_RESPONSE_FRAME;
    can0_parameter.self_reception = DISABLE;
    can0_parameter.mb_tx_abort_enable = DISABLE;
    can0_parameter.protocol_exception_enable = DISABLE;
    can0_parameter.local_priority_enable = DISABLE;
    can0_parameter.rx_private_filter_queue_enable = DISABLE;
    can0_parameter.edge_filter_enable = DISABLE;
    can0_parameter.rx_filter_order = CAN_RX_FILTER_ORDER_FIFO_FIRST;
    can0_parameter.memory_size = CAN_MEMSIZE_16_UNIT;
    can0_parameter.mb_public_filter = 0x00000000;
    can0_parameter.prescaler = 9;
    can0_parameter.resync_jump_width = 1;
    can0_parameter.prop_time_segment = 1;
    can0_parameter.time_segment_1 = 1;
    can0_parameter.time_segment_2 = 1;
    can_init(CAN0, &can0_parameter);

    can_arbitration_delay_bits_config(CAN0, 20);
    can_auto_busoff_recovery_disable(CAN0);
    can_time_sync_disable(CAN0);
    can_bsp_mode_config(CAN0, CAN_BSP_MODE_THREE_SAMPLES);
    can_bsp_syn_config(CAN0, CAN_BSP_TWO_STAGES_SYN);
    can_operation_mode_enter(CAN0, CAN_NORMAL_MODE);

    nvic_irq_enable(CAN0_Message_IRQn, 0, 0);

    /* user code [can0_init local 1] begin */

    /* user code [can0_init local 1] end */
}

/*!
    \brief      CAN0 deinitialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_can0_deinit(void)
{
    /* user code [can0_deinit local 0] begin */

    /* user code [can0_deinit local 0] end */
    can_deinit(CAN0);

    rcu_periph_clock_disable(RCU_CAN0);

    /* user code [can0_deinit local 1] begin */

    /* user code [can0_deinit local 1] end */
}

/*!
    \brief      FWDGT initialization function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void msd_fwdgt_init(void)
{
    /* user code [fwdgt_init local 0] begin */

    /* user code [fwdgt_init local 0] end */

    fwdgt_prescaler_value_config(FWDGT_PSC_DIV4);
    fwdgt_reload_value_config(4095);
    fwdgt_window_value_config(4095);

    /* user code [fwdgt_init local 1] begin */

    /* user code [fwdgt_init local 1] end */
}

/* user code [Public Functions Implementations] begin */

/* user code [Public Functions Implementations] end */

/* user code [Private Functions Implementations] begin */

/* user code [Private Functions Implementations] end */
