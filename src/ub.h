#ifndef __UB_H_
#define __UB_H_
#include <stdint.h>
#include "ubconfig.h"

enum UB_ERR_CODE { UB_OK, UB_ERROR };
typedef uint8_t UBSTATUS;

#define UB_ADDRESS_MASTER       1
#define UB_ADDRESS_BRIDGE       2

#define UB_ADDRESS_BROADCAST     ((1<<sizeof(ubaddress_t)*8)-1)     //all ones
#define UB_ADDRESS_MULTICAST     (1<<(sizeof(ubaddress_t)*8-1))     //first bit is one

#endif
