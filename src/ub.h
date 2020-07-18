#ifndef __UB_H_
#define __UB_H_
#include <stdint.h>
#include "ubconfig.h"

enum UB_ERR_CODE { UB_ERROR = 0, UB_OK = 1 };
typedef uint8_t UBSTATUS;

#define UB_ADDRESS_MASTER       1
#define UB_ADDRESS_BRIDGE       2

#define UB_ADDRESS_BROADCAST     ((1<<sizeof(ubaddress_t)*8)-1)     //all ones
#define UB_ADDRESS_MULTICAST     (1<<(sizeof(ubaddress_t)*8-1))     //first bit is one

// Moodlamp specific
#define CMD_POWER               0x10
#define CMD_BRIGHTNESS_UP       0x11
#define CMD_BRIGHTNESS_DOWN     0x12
#define CMD_FASTER              0x13
#define CMD_SLOWER              0x14
#define CMD_FULL_BRIGHTNESS     0x15
#define CMD_ZERO_BRIGHTNESS     0x16
#define CMD_PAUSE               0x17
#define CMD_SAVE                0x18
#define CMD_SLEEP               0x19
#define CMD_COLOR_UP            0x1A
#define CMD_COLOR_DOWN          0x1B
#define CMD_RED                 0x1C
#define CMD_GREEN               0x1D
#define CMD_BLUE                0x1E
#define CMD_COLOR_FULL          0x1F
#define CMD_COLOR_ZERO          0x20
#define CMD_SET_SCRIPT          0x21
#define CMD_GET_VERSION         'V'
#define CMD_GET_STATE           'G'
#define CMD_SET_SPEED           's'
#define CMD_SET_BRIGHTNESS      'D'
#define CMD_SET_STATE           'S'
#define CMD_SET_COLOR           'C'
#define CMD_RESET               'r'
#define CMD_SET_NAME            'N'
#define CMD_FADE                'F'
#define CMD_FADEMS              'M'
#define CMD_FADEMSALT           'T'
#define CMD_GET_VOLTAGE         'K'
#define CMD_GET_COLOR           'g'
#define CMD_FLASH               'f'

#define STATE_RUNNING           0
#define STATE_STANDBY           1
#define STATE_PAUSE             2
#define STATE_ENTERSTANDBY      3
#define STATE_LEAVESTANDBY      4
#define STATE_ENTERSLEEP        5
#define STATE_SLEEP             6
#define STATE_REMOTE            7
#define STATE_LOWBAT            8
#define STATE_ENTERPOWERDOWN    9
#define STATE_FLASH             10

#endif
