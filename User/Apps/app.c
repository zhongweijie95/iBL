#include "global_include.h"
#include "dronecan.h"
#include "can_handler.h"
#include "gd32_nvm_storage.h"
#include "bkp.h"

void jump_to_app(void);

reset_flag_t reset_flag = {0};
bkp_struct_t bkp_struct = {0};

uint8_t app_buffer[FLASH_SECTOR_SIZE_BYTES]; // per sector
uint32_t app_buffer_offset = 0;
uint32_t current_write_address = 0;
uint32_t last_erased_sector = 0xFFFFFFFF;

FuncRetCode OnFWUpdateGetFileSize(uint32_t size)
{
    // Erase procedure takes too long
    // const uint32_t ceil_size = ceil_to_n_divisible(size, FLASH_WRITE_GRAN_BITS / 8);
    // return Erase(FLASH_APP_START_ADDR, ceil_size);
    current_write_address = 0;
    memset(app_buffer, 0, sizeof(app_buffer));
    app_buffer_offset = 0;
    last_erased_sector = 0xFFFFFFFF;
    return OK;
}

// the offset is current ptr (before += len)
FuncRetCode OnFWUpdateReceiveBlock(uint32_t offset, uint8_t* data, uint32_t len)
{
    if(current_write_address == 0) current_write_address = FLASH_APP_START_ADDR + offset; // init
    while(len > 0)
    {
        const uint32_t remaining_bytes = sizeof(app_buffer) - app_buffer_offset;
        if(len <= remaining_bytes) // write then return
        {
            memcpy(app_buffer + app_buffer_offset, data, len);
            app_buffer_offset += len;
            break;
        }
        if(remaining_bytes > 0) // write to buffer tail
        {
            memcpy(app_buffer + app_buffer_offset, data, remaining_bytes);
            app_buffer_offset += remaining_bytes;
            data += remaining_bytes;
            len -= remaining_bytes;
        }
        // now app_buffer_offset == sizeof(buffer), remaining_bytes == 0, buffer full, execute erase + program
        const uint32_t erasing_sector = GetPage(current_write_address); // relative sector
        if(last_erased_sector != erasing_sector)
        {
            Erase(current_write_address, ceil_to_n_divisible(sizeof(app_buffer), FLASH_WRITE_GRAN_BITS / 8));
            last_erased_sector = erasing_sector;
        }
        // program
        Write_NoErase(current_write_address, app_buffer, ceil_to_n_divisible(sizeof(app_buffer), FLASH_WRITE_GRAN_BITS / 8));
        current_write_address += sizeof(app_buffer);
        app_buffer_offset = 0;
    }
    return OK;
}

FuncRetCode OnFWUpdateFinished(void)
{
    if(app_buffer_offset == 0) goto cleanup;
    const uint32_t erasing_sector = GetPage(current_write_address); // relative sector
    if(last_erased_sector != erasing_sector)
    {
        Erase(current_write_address, ceil_to_n_divisible(app_buffer_offset, FLASH_WRITE_GRAN_BITS / 8));
        last_erased_sector = erasing_sector;
    }
    Write_NoErase(current_write_address, app_buffer, ceil_to_n_divisible(app_buffer_offset, FLASH_WRITE_GRAN_BITS / 8));
cleanup:
    current_write_address = 0;
    // memset(app_buffer, 0, sizeof(app_buffer));
    app_buffer_offset = 0;
    last_erased_sector = 0xFFFFFFFF;

    // clean necessary bpr flags first
    read_bpr(&bkp_struct);
    bkp_struct.flags.bit.controllable_hardfault = false;
    bkp_struct.flags.bit.update_requested = false;
    bkp_struct.flags.bit.app_init_success = false;
    bkp_struct.controllable_hardfault_count = 0;
    write_bpr(&bkp_struct);
    RCU_RSTSCK |= RCU_RSTSCK_RSTFC; // clear reset flag

    jump_to_app(); // execute jump
    return OK;
}

FuncRetCode OnRestartNode(void)
{
    // clean necessary bpr flags first
    read_bpr(&bkp_struct);
    bkp_struct.flags.bit.controllable_hardfault = false;
    bkp_struct.flags.bit.update_requested = false;
    bkp_struct.flags.bit.app_init_success = false;
    bkp_struct.controllable_hardfault_count = 0;
    write_bpr(&bkp_struct);
    RCU_RSTSCK |= RCU_RSTSCK_RSTFC; // clear reset flag

    jump_to_app();
    return OK;
}

void app_main(void)
{
    /* setup systick timer for 10000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 10000U))
    {
        /* capture error */
        while (1);
    }

    SCB->CFSR = 0x00000000; // Clear previous CFSR flag
    dbg_periph_disable(DBG_TIMER0_HOLD);
    dbg_periph_disable(DBG_FWDGT_HOLD);
    dbg_periph_disable(DBG_WWDGT_HOLD);

    fwdgt_write_enable();
    fwdgt_prescaler_value_config(FWDGT_PSC_DIV4);
    fwdgt_reload_value_config(4095);
    fwdgt_window_value_config(4095);
    fwdgt_write_disable();

    fwdgt_enable();
    fwdgt_counter_reload();

    // read CRM_CTRLSTS, to determine reset flags
    reset_flag.reg = 0;
    if(RCU_RSTSCK & RCU_RSTSCK_LPRSTF) reset_flag.bit.low_power_reset = 1;
    if(RCU_RSTSCK & RCU_RSTSCK_WWDGTRSTF) reset_flag.bit.window_wdt_reset = 1;
    if(RCU_RSTSCK & RCU_RSTSCK_FWDGTRSTF) reset_flag.bit.wdt_reset = 1;
    if(RCU_RSTSCK & RCU_RSTSCK_SWRSTF) reset_flag.bit.software_reset = 1;
    if(RCU_RSTSCK & RCU_RSTSCK_PORRSTF) reset_flag.bit.power_on_reset = 1;
    if(RCU_RSTSCK & RCU_RSTSCK_EPRSTF) reset_flag.bit.nrst_reset = 1;
    RCU_RSTSCK |= RCU_RSTSCK_RSTFC; // clear reset flag

    // deciding jump
    if(reset_flag.bit.wdt_reset || reset_flag.bit.software_reset)
    {
        if(read_bpr(&bkp_struct))
        {
            if(reset_flag.bit.wdt_reset)
            {
                if(bkp_struct.flags.bit.app_init_success &&
                    bkp_struct.flags.bit.controllable_hardfault &&
                    bkp_struct.controllable_hardfault_count > 0 && bkp_struct.controllable_hardfault_count <= 3)
                {
                    jump_to_app();
                }
                // else: wait
            }
            else
            {
                if(bkp_struct.flags.bit.update_requested)
                {
                    if(bkp_struct.flags.bit.enable_can_resistor) gpio_bit_reset(CAN_RES_TRIG_B9_GPIO_PORT, CAN_RES_TRIG_B9_PIN);
                    else gpio_bit_set(CAN_RES_TRIG_B9_GPIO_PORT, CAN_RES_TRIG_B9_PIN);
                    // start firmware update
                    const uint8_t source_id = bkp_struct.file_server_node_id;
                    if(source_id >= CANARD_MIN_NODE_ID && source_id <= CANARD_MAX_NODE_ID)
                    {
                        // valid
                        // reset fw_update with given information, Read trigger: file_server_node_id != 0 && file_full_size > 0
                        // GetInfo trigger: file_server_node_id != 0 && file_full_size == 0
                        fw_update.file_server_node_id = 0;
                        memset(&fw_update, 0, sizeof(fw_update));
                        memcpy(fw_update.path, bkp_struct.dronecan_image_path, sizeof(bkp_struct.dronecan_image_path));
                        fw_update.file_server_node_id = source_id;
                    }
                    // invalid, exit and wait
                }
                else
                {
                    // jump.
                    initialize_bkp(&bkp_struct);
                    jump_to_app();
                }
            }
        }
        else // bkp_struct invalid, we shouldn't arrive here at first time. odd.
        {
            initialize_bkp(&bkp_struct);
            // wait
        }
    }
    else // other reset, or no flag is given (jump to here)
    {
        initialize_bkp(&bkp_struct);
        if(reset_flag.reg > 0) jump_to_app(); // only if we aren't jump here, we can jump out
    }

    memset(app_buffer, 0, sizeof(app_buffer));

    InitCAN(CAN0);

    InitDroneCAN();

    while(1)
    {
        ProcessDroneCAN_RT();
        if(timer_10khz)
        {
            ProcessDroneCAN_10KHz();
            if(timer_1ms++ >= 9)
            {
                if(timer_1s++ >= 999)
                {
                    uptime_sec++;
                    timer_1s = 0;
                }
                uwTick++;
                fwdgt_counter_reload(); // feed the dog
                ProcessDroneCAN_1KHz();
                timer_1ms = 0;
            }
            timer_10khz = 0;
        }
    }
}

void jump_to_app(void)
{
    void (*AppEntry)(void);
    const uint32_t app_addr = FLASH_APP_START_ADDR;

    if(((*(__IO uint32_t*)app_addr) & 0x2FFE0000) != 0x20000000) return; // INVALID sp address

    __disable_irq();

    // #1: Deinit all hardware used in BL
    // crm_reset();

    // Disable Systick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Disable USART1, CAN0, GPIOx
    usart_deinit(USART0);
    can_deinit(CAN0);
    gpio_deinit(GPIOA);
    gpio_deinit(GPIOB);
    gpio_deinit(GPIOC);
    gpio_deinit(GPIOD);
    gpio_deinit(GPIOE);

    // Disable all enabled interrupts
    for(uint8_t i = 0; i < 8; ++i)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    AppEntry = (void (*)(void)) (*((__IO uint32_t *) (app_addr + 4)));
    __set_MSP(*(__IO uint32_t*)app_addr); // main stack pointer MSP
    // __set_CONTROL(0);

    SCB->VTOR = app_addr;
    AppEntry();

    while(1);
}