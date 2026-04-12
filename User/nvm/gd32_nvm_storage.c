#include "gd32_nvm_address.h"
#include "func_ret_code.h"
#include <string.h>

uint32_t ceil_to_n_divisible(uint32_t num, uint32_t n)
{
    return (num + n - 1) / n * n;
}

#if defined(GD32G5X3)
uint32_t get_bank_number(uint32_t address)
{
    uint32_t base0_address;

    if(OB_DUAL_BANK_MODE != (FMC_OBCTL & FMC_OBCTL_DBS)){
        return FMC_BANK0;
    }else{
        base0_address = fmc_bank0_base_address_get();
        if((address >= base0_address) && (address < (base0_address + MAIN_FLASH_BANK_SIZE * 0x400U))){
            return FMC_BANK0;
        }else{
            return FMC_BANK1;
        }
    }
}
uint32_t GetPage(const uint32_t addr)
{
    uint32_t base_address = 0;
    const uint32_t bank = get_bank_number(addr);
    if(FMC_BANK0 == bank) base_address = fmc_bank0_base_address_get();
    else base_address = fmc_bank1_base_address_get();
    return (addr - base_address) / fmc_page_size_get();
}
uint32_t GetPageSize(const uint32_t page_number)
{
    (void)page_number;
    return fmc_page_size_get();
}

FuncRetCode Erase(uint32_t addr, size_t size) // erase sectors
{
    if(addr < FLASH_APP_START_ADDR || addr >= FLASH_USER_START_ADDR) return ACCESS_VIOLATION;
    const uint32_t page_size = fmc_page_size_get();
    uint32_t curr_addr = addr;
    uint32_t size_to_erase = 0;
    uint32_t base_address = 0;
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_ENDF | FMC_FLAG_RPERR | FMC_FLAG_OBERR | FMC_FLAG_PGERR | FMC_FLAG_WPERR | FMC_FLAG_OPRERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR | FMC_FLAG_PGAERR);
    while(size_to_erase < size)
    {
        const uint32_t bank = get_bank_number(curr_addr);
        if(FMC_BANK0 == bank) base_address = fmc_bank0_base_address_get();
        else base_address = fmc_bank1_base_address_get();
        const uint32_t page_offset = (curr_addr - base_address) / page_size;
        fmc_page_erase(bank, page_offset);
        fmc_flag_clear(FMC_FLAG_ENDF | FMC_FLAG_RPERR | FMC_FLAG_OBERR | FMC_FLAG_PGERR | FMC_FLAG_WPERR | FMC_FLAG_OPRERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR | FMC_FLAG_PGAERR);
        size_to_erase += page_size;
        curr_addr += page_size;
    }
    fmc_lock();
    return OK;
}

FuncRetCode Write_NoErase(uint32_t addr, const uint8_t* buffer, size_t size)
{
    if(addr < FLASH_APP_START_ADDR || addr >= FLASH_USER_START_ADDR) return ACCESS_VIOLATION;
    if(size % (FLASH_WRITE_GRAN_BITS / 8)) return ACCESS_VIOLATION;
    volatile uint64_t write_data = 0; // DOUBLE WORD
    volatile uint64_t read_data = 0;
    fmc_unlock();
    while(size)
    {
        memcpy((uint64_t*)&write_data, buffer, 8);
        read_data = *(uint64_t*)addr;
        if(write_data != read_data)
        {
            uint8_t retry_time = 0;
            while(retry_time < 3)
            {
                if(fmc_doubleword_program(addr, write_data) == FMC_READY) break;
                retry_time++;
            }
            read_data = *(uint64_t*)addr;
            if(write_data != read_data || retry_time >= 3)
            {
                fmc_lock();
                return HARDWARE_ERROR;
            }
        }
        addr += 8;
        buffer += 8;
        if(size >= 8) size -= 8;
        else break;
    }
    fmc_lock();
    return OK;
}

FuncRetCode Read(uint32_t addr, uint8_t *buffer, const size_t size)
{
    if(addr < FLASH_APP_START_ADDR || addr >= FLASH_USER_START_ADDR) return ACCESS_VIOLATION;
    for(size_t i = 0; i < size; i++, addr++, buffer++)
    {
        *buffer = *(uint8_t*)addr;
    }
    return OK;
}

#endif