#ifndef __PACKET_H_
#define __PACKET_H_
#include <stdint.h>
#include "ubconfig.h"

struct ubheader_t{
    ubaddress_t src;
    ubaddress_t dest;
    uint8_t flags;
    uint8_t cls;
    uint8_t len;
};

struct ubpacket_t{
    struct ubheader_t header;
    char data[UB_PACKETLEN - sizeof(struct ubheader_t)];
};

#define UB_PACKET_HEADER        (sizeof(struct ubheader_t)

#define UB_PACKET_ACK           (1<<0)
#define UB_PACKET_SEQ           (1<<1)
#define UB_PACKET_NOACK         (1<<3)
#define UB_PACKET_MGT           (1<<4)
#define UB_PACKET_UNSOLICITED   (1<<5)  //when the node starts talking
#define UB_PACKET_ACKSEQ        (1<<6)

#define UB_PACKET_IDLE          0
#define UB_PACKET_BUSY          1
#define UB_PACKET_NEW           4

#endif
