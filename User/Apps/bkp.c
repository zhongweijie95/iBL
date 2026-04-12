#include "bkp.h"
#include <assert.h>

void initialize_bkp(bkp_struct_t* dst)
{
    memset(dst, 0, sizeof(bkp_struct_t));
    dst->flags.bit.bootloader_presented = 1;
    dst->version.major = (uint8_t)YEAR();
    dst->version.minor = (uint8_t)MONTH();
    const uint32_t vcs_commit = (uint32_t)dec_to_hex_byte(MONTH()) << 24 |
                                (uint32_t)dec_to_hex_byte(DAY()) << 16 |
                                (uint32_t)dec_to_hex_byte(HOUR()) << 8 |
                                (uint32_t)dec_to_hex_byte(MINUTE()) << 0;
    memcpy(dst->version.vcs_commit, &vcs_commit, sizeof(uint32_t));
    write_bpr(dst);
}

// bkp_struct_t size: 18, BPR_DATA1 -> BPR_DATA9, little-endian
bool read_bpr(bkp_struct_t* dst)
{
    static_assert(sizeof(bkp_struct_t) == 18);

    memset(dst, 0, sizeof(bkp_struct_t));
    uint8_t buffer[sizeof(bkp_struct_t)];

    uint32_t temp = 0;

    temp = RTC_BKP0;
    memcpy(buffer, &temp, 4);

    temp = RTC_BKP1;
    memcpy(buffer + 4, &temp, 4);

    temp = RTC_BKP2;
    memcpy(buffer + 8, &temp, 4);

    temp = RTC_BKP3;
    memcpy(buffer + 12, &temp, 4);

    temp = RTC_BKP4;
    memcpy(buffer + 16, &temp, 2);

    const uint8_t calc_crc8 = get_crc8(buffer, sizeof(buffer) - 1);
    if(calc_crc8 == buffer[sizeof(buffer) - 1]) // crc8 matched
    {
        memcpy(dst, buffer, sizeof(bkp_struct_t));
        return true;
    }
    return false;
}

void write_bpr(bkp_struct_t* src)
{
    static_assert(sizeof(bkp_struct_t) == 18);

    uint32_t temp = 0;

    const uint8_t* ptr = (const uint8_t*)src;

    src->crc8 = get_crc8(ptr, sizeof(bkp_struct_t) - 1);

    // Unlock BKP
    pmu_backup_write_enable();

    memcpy(&temp, ptr, 4);
    RTC_BKP0 = temp;
    ptr += 4;
    temp = 0;

    memcpy(&temp, ptr, 4);
    RTC_BKP1 = temp;
    ptr += 4;
    temp = 0;

    memcpy(&temp, ptr, 4);
    RTC_BKP2 = temp;
    ptr += 4;
    temp = 0;

    memcpy(&temp, ptr, 4);
    RTC_BKP3 = temp;
    ptr += 4;
    temp = 0;

    memcpy(&temp, ptr, 2);
    RTC_BKP4 = temp;
}