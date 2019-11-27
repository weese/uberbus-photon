// #include <util/crc16.h>
#include "ubcrc.h"
#include "ubconfig.h"

/*uint16_t ubcrc16_frame(struct frame * f)
{
    uint16_t crc = 0xFFFF;
    uint8_t i = 0;

    crc = _crc_ccitt_update(crc, f->len);
    for(i=0;i<f->len;i++){
        crc = _crc_ccitt_update(crc, f->data[i]);
    }

    return crc;
}*/

uint16_t _crc_ccitt_update (uint16_t crc, uint8_t data)
{
    data ^= crc & 0xff;
    data ^= data << 4;

    return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) 
            ^ ((uint16_t)data << 3));
}

uint16_t ubcrc16_data(void *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    uint8_t i = 0;

    crc = _crc_ccitt_update(crc, len);
    for(i=0; i<len; i++){
        crc = _crc_ccitt_update(crc, ((uint8_t*)data)[i]);
    }

    return crc;
}
