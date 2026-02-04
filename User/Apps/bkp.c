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
    static_assert(sizeof(bkp_struct_t) == 18, "");

    memset(dst, 0, sizeof(bkp_struct_t));
    uint8_t buffer[sizeof(bkp_struct_t)];
    volatile uint32_t temp = 0;

    temp = bpr_data_read(BPR_DATA1);
    buffer[0] = (uint8_t)(temp & 0xFF);
    buffer[1] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA2);
    buffer[2] = (uint8_t)(temp & 0xFF);
    buffer[3] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA3);
    buffer[4] = (uint8_t)(temp & 0xFF);
    buffer[5] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA4);
    buffer[6] = (uint8_t)(temp & 0xFF);
    buffer[7] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA5);
    buffer[8] = (uint8_t)(temp & 0xFF);
    buffer[9] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA6);
    buffer[10] = (uint8_t)(temp & 0xFF);
    buffer[11] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA7);
    buffer[12] = (uint8_t)(temp & 0xFF);
    buffer[13] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA8);
    buffer[14] = (uint8_t)(temp & 0xFF);
    buffer[15] = (uint8_t)(temp >> 8);
    temp = bpr_data_read(BPR_DATA9);
    buffer[16] = (uint8_t)(temp & 0xFF);
    buffer[17] = (uint8_t)(temp >> 8);

    // crc calculation
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
    static_assert(sizeof(bkp_struct_t) == 18, "");

    volatile uint32_t temp = 0;

    const uint8_t* ptr = (const uint8_t*)src;

    src->crc8 = get_crc8(ptr, sizeof(bkp_struct_t) - 1);

    // Unlock BPR first
    CRM->apb1en_bit.bpren = 1;
    CRM->apb1en_bit.pwcen = 1;
    PWC->ctrl_bit.bpwen = 1;

    temp = (uint32_t)((uint8_t)*(ptr + 0)) | (uint32_t)((uint8_t)*(ptr + 1)) << 8;
    bpr_data_write(BPR_DATA1, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 2)) | (uint32_t)((uint8_t)*(ptr + 3)) << 8;
    bpr_data_write(BPR_DATA2, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 4)) | (uint32_t)((uint8_t)*(ptr + 5)) << 8;
    bpr_data_write(BPR_DATA3, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 6)) | (uint32_t)((uint8_t)*(ptr + 7)) << 8;
    bpr_data_write(BPR_DATA4, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 8)) | (uint32_t)((uint8_t)*(ptr + 9)) << 8;
    bpr_data_write(BPR_DATA5, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 10)) | (uint32_t)((uint8_t)*(ptr + 11)) << 8;
    bpr_data_write(BPR_DATA6, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 12)) | (uint32_t)((uint8_t)*(ptr + 13)) << 8;
    bpr_data_write(BPR_DATA7, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 14)) | (uint32_t)((uint8_t)*(ptr + 15)) << 8;
    bpr_data_write(BPR_DATA8, temp);

    temp = (uint32_t)((uint8_t)*(ptr + 16)) | (uint32_t)((uint8_t)*(ptr + 17)) << 8;
    bpr_data_write(BPR_DATA9, temp);

    // PWC->ctrl_bit.bpwen = 0; // lock BPR // lock BPR cause lock of RTC
}