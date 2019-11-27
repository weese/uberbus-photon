// This #include statement was automatically added by the Particle IDE.
#include "ubcrc.h"

// This #include statement was automatically added by the Particle IDE.
#include "ubrf12.h"
#include "ubconfig.h"
#include "ubpacket.h"
#include "ubclasses.h"
#include "ubslavemgt.h"
#include "ub.h"

// reset the system after 10 seconds if the application is unresponsive
ApplicationWatchdog wd(10000, System.reset);


// Log message to cloud, message is a printf-formatted string
void debug(String message, int value = 0) {
    char msg [250];
    sprintf(msg, message.c_str(), value);
    Particle.publish("DEBUG", msg);
}

void rf_init(void)
{
    // ubleds_rx();
    ubrf12_init(23);
    ubrf12_setfreq(RF12FREQ(434.321));
    ubrf12_setbandwidth(4, 1, 4);     // 200kHz Bandbreite, -6dB Verstärkung, DRSSI threshold: -79dBm
    ubrf12_setbaud(19200);
    ubrf12_setpower(0, 6);            // 1mW Ausgangangsleistung, 120kHz Frequenzshift
    // ubrf12_rxstart();
    debug("rxstart %i", ubrf12_rxstart());
    // ubleds_rxend();
}


const char * setcolor  = "C\xff\x00\x00";
const char * setcolor2 = "C\x00\x00\xff";

void setup() {
    debug("Start");
    delay(5);
    rf_init();

    // while (rf12_txfinished());  // HIER BLEIBT ER HÄNGEN
    
    debug("Done");
}

bool alternate = false;

struct ubpacket_t packet;

unsigned short l = 0;

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


String deviceList[32];

unsigned char nextDeviceAddr = 2;

unsigned char addDevice(char *name) {
    String sname = name;
    for (unsigned i = 0; i < 32; ++i) {
        if (deviceList[i].equals(name)) {
            return i;
        }
    }
    unsigned char addr = nextDeviceAddr;
    deviceList[addr] = name;
    nextDeviceAddr = (nextDeviceAddr + 1) & 31;
    return addr;
}

void assignAddr(char *name, unsigned char addr) {
    unsigned char len = strlen(name);

    debug("assign %d -> " + String(name), addr);

    strcpy(packet.data + 2, name);
    packet.header.src = UB_ADDRESS_MASTER;
    packet.header.dest = UB_ADDRESS_BROADCAST;
    packet.header.flags = UB_PACKET_MGT | UB_PACKET_NOACK;
    packet.header.cls = UB_CLASS_MOODLAMP;
    packet.header.len = len + 3;
    packet.data[0] = 'S';
    packet.data[1] = addr;
    ubrf_sendPacket(&packet);
}

void setColor(unsigned char addr, unsigned char r, unsigned char g, unsigned char b) {
    packet.header.src = UB_ADDRESS_MASTER;
    packet.header.dest = addr;
    packet.header.flags = UB_PACKET_NOACK;
    packet.header.cls = UB_CLASS_MOODLAMP;
    packet.header.len = 5;
    packet.data[0] = 'C';
    packet.data[1] = r;
    packet.data[2] = g;
    packet.data[3] = b;
    packet.data[4] = '\n';
    ubrf_sendPacket(&packet);
}


void loop() {
    unsigned char len = ubrf12_rxfinish(&packet);
    if (len != 255) {
        char b[6];
        String m = "read: ";
        for (unsigned char i = 0; i < len; ++i) {
            sprintf(b, "%02x ", ((char*)&packet)[i]);
            m += b;
        }
        debug(m);
        
        if (packet.header.dest == UB_ADDRESS_BROADCAST || packet.header.dest == UB_ADDRESS_MASTER) {
            unsigned char addr = packet.header.src;
            switch (packet.data[0]) {
                case MGT_DISCOVER:
                    packet.data[packet.header.len] = 0;
                    addr = addDevice(packet.data + 7);
                    // delay(500);
                    assignAddr(packet.data + 7, addr);
                    // delay(500);
                case MGT_IDENTIFY:
                    // We could connect the lamp now, but then it would no longer reveal its ID.
                    // So, if the bridge restarts, all connected lamps would keep their address and the bridge wouldn't know their IDs.
                    // That could lead to address clashes. So we simply let them repeat their names after we set their addresses.
                    //
                    // packet.header.dest = packet.header.src;
                    // packet.header.src = UB_ADDRESS_MASTER;
                    // packet.header.flags = UB_PACKET_MGT | UB_PACKET_NOACK;
                    // packet.header.cls = UB_CLASS_MOODLAMP;
                    // packet.header.len = 1;
                    // packet.data[0] = 'O';
                    // ubrf_sendPacket(&packet);
                    // // delay(500);
                    // break;
                case MGT_ALIVE:
                    setColor(addr, rand() & 0xff, rand() & 0xff, rand() & 0xff);
                    // delay(500);
                    break;
            }
        }    
    }
    ubrf12_rxstart();

    // if (l++ == 2000) {
    //     l = 0;
    //     // unsigned short x = ubrf12_trans(0x0000);

    //     debug("isr %0x", isr_counter);
    //     // debug("tick %0x", x);
    // }
    // if (l == 1000) {
        // if (alternate)
        //     ubrf12_txstart(setcolor, 4);
        // else
        //     ubrf12_txstart(setcolor2, 4);
        // alternate = !alternate;
    // }
    // if (RF12_status.Tx && digitalRead(RF12_IRQ_PIN) != LOW) {
    //     rf12_isr();
    // }
    // delay(1000);
}