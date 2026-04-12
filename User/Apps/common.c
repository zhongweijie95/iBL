#include "global_include.h"

uint8_t dec_to_hex_byte(const uint8_t dec)
{
    if(dec > 99) return 0;
    const uint8_t tens = dec / 10;
    const uint8_t units = dec % 10;
    return (tens << 4) | units;
}

volatile uint32_t uptime_sec;

uint32_t GetUptimeSeconds(void)
{
    return uptime_sec;
}

volatile uint32_t uwTick;

volatile uint8_t timer_1ms;

volatile uint16_t timer_1s;

volatile uint8_t timer_10khz;

uint32_t GetCurrentTickCount(void)
{
    return uwTick;
}

uint32_t GetSerialNumber(void)
{
#if defined(GD32G5X3)
#define UID_BASE_1      0x1FFFB3E8
#define UID_BASE_2      0x1FFFB3EC
#define UID_BASE_3      0x1FFFB3F0
    const uint8_t *p = (const uint8_t *)UID_BASE_1;
    return ((uint32_t)p[10]<<24) | ((uint32_t)p[1]<<8) | p[0]
        | (((p[9]&0x80)|((p[4]&0x03)<<5)|((p[3]&0xF0)>>3)|(p[2]&0x01))<<16);
#else
#warning "_get_sn() unavailable for unknown chip"
#endif
}

extern const volatile uint32_t __firmware_start;
extern const volatile uint32_t __firmware_end;

uint32_t GetFirmwareSizeBytes(void)
{
    const uint32_t start_addr = (uint32_t)&__firmware_start;
    const uint32_t end_addr = (uint32_t)&__firmware_end;
    const uint32_t firmware_size = end_addr - start_addr;
    return firmware_size;
}

#define CRC64_ECMA_POLY (0xC96C5795D7870F42ULL)
#define CRC64_ECMA_INIT (0xFFFFFFFFFFFFFFFFULL)

static uint64_t get_crc64(const uint8_t* data, size_t len)
{
    // static constexpr uint64_t CRC64_ECMA_POLY = 0xC96C5795D7870F42ULL;
    // static constexpr uint64_t CRC64_ECMA_INIT = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t crc = CRC64_ECMA_INIT;
    while(len--)
    {
        crc ^= (uint64_t)(*data++);
        for(uint8_t i = 0; i < 8; i++)
        {
            if(crc & 1) crc = (crc >> 1) ^ CRC64_ECMA_POLY;
            else crc >>= 1;
        }
    }
    return ~crc;
}

uint64_t _crc_64 = 0;

uint64_t GetFirmwareCRC64(void)
{
    if(_crc_64 == 0) _crc_64 = get_crc64((const uint8_t*)&__firmware_start, GetFirmwareSizeBytes());
    return _crc_64;
}

#define CRC8_POLY (0x07)

uint8_t get_crc8(const uint8_t* data, const size_t len)
{
    uint8_t crc = 0x00;
    for(size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for(uint8_t bit = 0; bit < 8; bit++)
        {
            if(crc & 0x80) crc = (crc << 1) ^ CRC8_POLY;
            else crc <<= 1;
        }
    }
    return crc;
}

int YEAR(void) { return (((__DATE__[9]-'0')) * 10 + (__DATE__[10]-'0')); }

int MONTH(void)
{
    return (
            __DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n' ? 1 :
            __DATE__[0] == 'F' ? 2 :
            __DATE__[0] == 'M' && __DATE__[2] == 'r' ? 3 :
            __DATE__[0] == 'A' && __DATE__[1] == 'p' ? 4 :
            __DATE__[0] == 'M' ?  5 :
            __DATE__[0] == 'J' && __DATE__[1] == 'u' ? 6 :
            __DATE__[0] == 'J' ? 7 :
            __DATE__[0] == 'A' ? 8 :
            __DATE__[0] == 'S' ? 9 :
            __DATE__[0] == 'O' ? 10 :
            __DATE__[0] == 'N' ? 11 : 12
    );
}

int DAY(void)
{
    return ((__DATE__[4] == ' ' ? 0 : __DATE__[4]-'0') * 10 + (__DATE__[5]-'0'));
}

int HOUR(void)
{
    return ((__TIME__[0]-'0') * 10 + __TIME__[1]-'0');
}

int MINUTE(void)
{
    return((__TIME__[3]-'0') * 10 + __TIME__[4]-'0');
}