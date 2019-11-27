// This #include statement was automatically added by the Particle IDE.
#include "ubleds.h"

// This #include statement was automatically added by the Particle IDE.
#include "ubrf.h"

// This #include statement was automatically added by the Particle IDE.
#include <SparkJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

// This #include statement was automatically added by the Particle IDE.
//#include <particle-hap.h>

// This #include statement was automatically added by the Particle IDE.
#include "ubcrc.h"

// This #include statement was automatically added by the Particle IDE.
#include "ubrf12.h"
#include "ubconfig.h"
#include "ubpacket.h"
#include "ubclasses.h"
#include "ubslavemgt.h"
#include "ub.h"


#define MAX_DEVICES 32

#define HASS_BROKER "192.168.100.1"
#define HASS_ACCESS_USER "mqtt_user"
#define HASS_ACCESS_PASS "mqtt_pass"
#define HASS_DEVICE_ID "light/moodlamp"
#define HASS_TOPIC_STATE "home/" HASS_DEVICE_ID "/status"
#define HASS_TOPIC_SET "home/" HASS_DEVICE_ID "/set"


// reset the system after 10 seconds if the application is unresponsive
ApplicationWatchdog wd(10000, System.reset);


// Log message to cloud, message is a printf-formatted string
void debug(String message, int value = 0) {
    char msg [500];
    sprintf(msg, message.c_str(), value);
    Particle.publish("DEBUG", msg);
}

struct ubpacket_t packet;

struct rgb_t {
    uint8_t r, g, b;
    
    rgb_t() {}
    rgb_t(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}
    
    rgb_t nscale8(uint8_t scale) {
        return rgb_t(
            ((int)r * (int)(scale) ) >> 8,
            ((int)g * (int)(scale) ) >> 8,
            ((int)b * (int)(scale) ) >> 8);
    }

    rgb_t nscale8_video(uint8_t scale) {
        uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
        return rgb_t(
            (r == 0) ? 0 : (((int)r * (int)(scale) ) >> 8) + nonzeroscale,
            (g == 0) ? 0 : (((int)g * (int)(scale) ) >> 8) + nonzeroscale,
            (b == 0) ? 0 : (((int)b * (int)(scale) ) >> 8) + nonzeroscale);
    }
};

struct moodlamp_t {
    String id;
    rgb_t rgb;
    uint8_t brightness;
    bool isOn;
    
    moodlamp_t():
        rgb(255, 255, 255),
        brightness(255),
        isOn(true) {}
};

moodlamp_t deviceList[MAX_DEVICES];

bool isUnknownDevice(char *id, uint8_t addr) {
    return !(addr < MAX_DEVICES && deviceList[addr].id.equals(id));
}

unsigned char addDevice(char *id) {
    for (unsigned char i = 2; i < MAX_DEVICES; ++i) {
        if (deviceList[i].id.equals(id)) {
            return i;
        }
    }
    for (unsigned char i = 2; i < MAX_DEVICES; ++i) {
        if (deviceList[i].id.equals("")) {
            deviceList[i].id = id;
            return i;
        }
    }
    return 0;
}

void assignAddr(char *id, unsigned char addr) {
    unsigned char len = strlen(id);

    debug("assign %d -> " + String(id), addr);

    strcpy(packet.data + 2, id);
    packet.header.src = UB_ADDRESS_MASTER;
    packet.header.dest = UB_ADDRESS_BROADCAST;
    packet.header.flags = UB_PACKET_MGT | UB_PACKET_NOACK;
    packet.header.cls = UB_CLASS_MOODLAMP;
    packet.header.len = len + 3;
    packet.data[0] = 'S';
    packet.data[1] = addr;
    ubrf_sendPacket(&packet);
}

void setColor(unsigned char addr, rgb_t const &rgb) {
    packet.header.src = UB_ADDRESS_MASTER;
    packet.header.dest = addr;
    packet.header.flags = UB_PACKET_NOACK;
    packet.header.cls = UB_CLASS_MOODLAMP;
    packet.header.len = 5;
    packet.data[0] = 'C';
    packet.data[1] = rgb.r;
    packet.data[2] = rgb.g;
    packet.data[3] = rgb.b;
    packet.data[4] = '\n';
    ubrf_sendPacket(&packet);
}

MQTT clientHass(HASS_BROKER, 1883, callbackHass);

void sendStateHass() {
    moodlamp_t &lamp = deviceList[2];

    StaticJsonBuffer<250> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& color = root.createNestedObject("color");
    color["r"] = lamp.rgb.r;
    color["g"] = lamp.rgb.g;
    color["b"] = lamp.rgb.b;
    root["state"] = (lamp.isOn) ? "ON" : "OFF";
    root["brightness"] = lamp.brightness;

    char buffer[250];
    root.printTo(buffer, sizeof(buffer));
    // debug(buffer);
    clientHass.publish(HASS_TOPIC_STATE, buffer, true);
}

void updateLamp(uint8_t addr)
{
    moodlamp_t &lamp = deviceList[addr];
    setColor(addr, lamp.rgb.nscale8_video(lamp.isOn ? lamp.brightness : 0));
    // sendStateHass();
}

// Callback signature for MQTT subscriptions
void callbackHass(char* topic, uint8_t* payload, unsigned int length) {
    /*Parse the command payload.*/
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *)payload);

    uint8_t addr = 2;
    moodlamp_t &lamp = deviceList[addr];

    if (root.containsKey("state")) {
        lamp.isOn = (strcmp(root["state"], "ON") == 0);
    }
    if (root.containsKey("brightness")) {
        lamp.brightness = root["brightness"];
    }
    if (root.containsKey("color")) {
        JsonObject& rgb = root["color"];
        lamp.rgb = rgb_t(rgb["r"], rgb["g"], rgb["b"]);
    }
    updateLamp(addr);
}

bool connectHassOnDemand() {
    if (clientHass.isConnected())
        return true;

    clientHass.connect(
        HASS_DEVICE_ID,
        HASS_ACCESS_USER,
        HASS_ACCESS_PASS);
    
    bool bConn = clientHass.isConnected();
    if (bConn) {
        debug("Connected to HASS");
        clientHass.subscribe(HASS_TOPIC_SET);
    }
    return bConn;
}

void loopHASS() {
    if (!connectHassOnDemand()) {
        return;
    }
    // Loop the MQTT client
    clientHass.loop();
}

void setup() {
    ubrf_init();
    connectHassOnDemand();
    debug("Initialized");
}

void loop() {
    if (ubrf_getPacket(&packet) == UB_OK) {
        unsigned char len = sizeof(struct ubheader_t) + packet.header.len + 2;
        char b[6];
        String m = "read: ";
        for (unsigned char i = 0; i < len; ++i) {
            sprintf(b, "%02x ", ((char*)&packet)[i]);
            m += b;
        }
        debug(m);
        
        if (packet.header.dest == UB_ADDRESS_BROADCAST || packet.header.dest == UB_ADDRESS_MASTER) {
            unsigned char addr = packet.header.src;
            unsigned char newAddr;
            switch (packet.data[0]) {
                case MGT_DISCOVER:
                    // null-terminate id string
                    packet.data[packet.header.len] = 0;
                    // it's a lamp wihtout address, so assign one
                    newAddr = addDevice(packet.data + 7);
                    assignAddr(packet.data + 7, newAddr);
                    // send default rgb values to lamp and MQTT server
                        delay(100);
                    updateLamp(newAddr);
                        delay(100);
                    break;
                case MGT_IDENTIFY:
                    // null-terminate id string
                    packet.data[packet.header.len] = 0;
                    if (isUnknownDevice(packet.data + 1, addr)) {
                        newAddr = addDevice(packet.data + 1);
                        // if we haven't the lamp in our records, we need to assign a new address
                        assignAddr(packet.data + 1, newAddr);
                        delay(100);
                        // send default rgb values to lamp and MQTT server
                        updateLamp(newAddr);
                        delay(100);
                    }
                    break;
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
                    // break;
                case MGT_ALIVE:
                    // setColor(addr, rand() & 0xff, rand() & 0xff, rand() & 0xff);
                    break;
            }
        }    
    }
    loopHASS();
    ubrf12_rxstart();

}