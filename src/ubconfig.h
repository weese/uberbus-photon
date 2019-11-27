#ifndef _UBCONFIG_H_
#define _UBCONFIG_H_
#include <stdint.h>

#define RF_CHANNEL 23

#define UB_MAXMULTICAST 8

typedef uint8_t ubaddress_t;
#define UB_NODEMAX      128

#define UB_PACKET_TIMEOUT   100
#define UB_RF_TIMEOUT       100
#define UB_RS485_TIMEOUT   (UB_INTERVAL + 200)
#define UB_PACKET_RETRIES   6

#define UB_PACKETLEN        50

#define UB_ESCAPE     '\\'
#define UB_NONE         '0'
#define UB_START        '1'
#define UB_STOP         '2'
#define UB_DISCOVER     '3'
#define UB_QUERY        '4'
#define UB_BOOTLOADER   '5'

#endif
