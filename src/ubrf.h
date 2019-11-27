#ifndef _UBRF_H_
#define _UBRF_H_
#include "ub.h"
#include "ubpacket.h"

//init the module
void ubrf_init(void);

//check for a packet in the buffer
UBSTATUS ubrf_getPacket(struct ubpacket_t * packet);

//send a packet
//return UB_OK when a slot is free
UBSTATUS ubrf_sendPacket(struct ubpacket_t * packet);

#endif
