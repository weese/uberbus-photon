#include "ubconfig.h"
#include "ubrf.h"
#include "ub.h"
#include "ubpacket.h"
#include "ubcrc.h"
#include "ubrf12.h"
#include "ubleds.h"

#include "application.h"

// Log message to cloud, message is a printf-formatted string
void debug(String message, int value = 0);


enum UBRF_STATE {   UBRF_IDLE,
                    UBRF_WAITFREE,
                    UBRF_WAITRND,
                    UBRF_SEND };
uint8_t ubrf_state;
uint8_t packetdata[UB_PACKETLEN+2];    //+ 2 byte crc
uint8_t packetlen;

void ubrf_init(void)
{
    ubleds_rx();
    ubrf_state = UBRF_IDLE;
    ubrf12_init(RF_CHANNEL);
    ubrf12_setfreq(RF12FREQ(434.321));
    ubrf12_setbandwidth(4, 1, 4);     // 200kHz Bandbreite,
    //-6dB Verstärkung, DRSSI threshold: -79dBm
    ubrf12_setbaud(19200);
    ubrf12_setpower(0, 6);            // 1mW Ausgangangsleistung,120kHz Frequenzshift
    ubrf12_rxstart();
    ubleds_rxend();
}

UBSTATUS ubrf_getPacket(struct ubpacket_t * packet)
{
    uint8_t *buf = (uint8_t*)packet;
    uint8_t len = ubrf12_rxfinish(buf);
    // ubrf12_rxstart();
    if( len == 255 || len < 3 ){
        return UB_ERROR;
    }
    uint16_t crc = ubcrc16_data(buf, len-2);
    if( (crc>>8) == buf[len-2] &&
        (crc&0xFF) == buf[len-1] ){
        return UB_OK;
    }
    return UB_ERROR;
}

//send a frame with data.
UBSTATUS ubrf_sendPacket(struct ubpacket_t * packet)
{
    // if( ubrf_state == UBRF_IDLE ){
    //     ubrf_state = UBRF_WAITFREE;

        uint8_t len = packet->header.len;
        uint16_t crc = ubcrc16_data(packet, sizeof(packet->header) + len);
        packet->data[len++] = crc>>8;
        packet->data[len++] = crc&0xFF;
        len += sizeof(packet->header);

        char b[6];
        String m = "send: ";
        for (unsigned char i = 0; i < len; ++i) {
            sprintf(b, "%02x ", ((char*)packet)[i]);
            m += b;
        }
        debug(m);

        while (!ubrf12_txfinished()) {}
        
        ubrf12_txstart(packet, len);
        return UB_OK;
    // }
    // return UB_ERROR;
}

