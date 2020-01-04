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
#define ALIVE_TIMEOUT 20000
#define STARTUP_TOLERANCE 30000

#define HASS_BROKER "192.168.100.1"
#define HASS_ACCESS_USER "mqtt_user"
#define HASS_ACCESS_PASS "mqtt_pass"
#define HASS_TOPIC_PREFIX "homeassistant/light/"
#define HASS_TOPIC_STATE_SUFFIX "/status"
#define HASS_TOPIC_SET_SUFFIX "/set"
#define HASS_TOPIC_CONFIG_SUFFIX "/config"


// reset the system after 10 seconds if the application is unresponsive
// ApplicationWatchdog wd(10000, System.reset);

SerialDebugOutput debugOutput(115200, ALL_LEVEL);


// Log message to cloud, message is a printf-formatted string
#define debug(...) { char msg[500]; sprintf(msg, __VA_ARGS__); Particle.publish("DEBUG", msg); Serial.printf("%s\n", msg); }

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

enum mqttState_t { unknown, unregistered, registered };

struct moodlamp_t {
    String id;
    rgb_t rgb;
    uint8_t brightness;
    bool isOn;
    mqttState_t mqttState;
    unsigned long lastSeen;
    
    moodlamp_t():
        rgb(255, 255, 255),
        brightness(255),
        isOn(true),
        lastSeen(0),
        mqttState(unknown) {}
};

moodlamp_t deviceList[MAX_DEVICES];
String particleDeviceName;

bool isKnownDevice(char *id, uint8_t addr) {
    return addr < MAX_DEVICES && deviceList[addr].id.equals(id);
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
    debug("Assign %d -> %s", addr, id);
    memmove(packet.data + 2, id, len + 1);
    packet.header.src = UB_ADDRESS_MASTER;
    packet.header.dest = UB_ADDRESS_BROADCAST;
    packet.header.flags = UB_PACKET_MGT | UB_PACKET_NOACK;
    packet.header.cls = UB_CLASS_MOODLAMP;
    packet.header.len = len + 3;
    packet.data[0] = 'S';
    packet.data[1] = addr;
    if (ubrf_sendPacket(&packet) == UB_ERROR) {
        debug("Couldn't send ASSIGN packet");
    }
}

void setColor(unsigned char addr, rgb_t const &rgb) {
    packet.header.src = UB_ADDRESS_MASTER;
    packet.header.dest = addr;
    packet.header.flags = UB_PACKET_NOACK;
    packet.header.cls = UB_CLASS_MOODLAMP;
    packet.header.len = 5;
    packet.data[0] = 'M';
    packet.data[1] = rgb.r;
    packet.data[2] = rgb.g;
    packet.data[3] = rgb.b;
    packet.data[4] = 1;
    packet.data[5] = 0;
    packet.data[6] = '\n';
    if (ubrf_sendPacket(&packet) == UB_ERROR) {
        debug("Couldn't send SET_COLOR packet");
    }
}

void callbackHass(char* topic, uint8_t* payload, unsigned int length);
MQTT clientHass(HASS_BROKER, 1883, callbackHass);

void sendStateHass(uint8_t addr) {
    debug("enter sendStateHass %d", addr);
    moodlamp_t &lamp = deviceList[addr];

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
    String topic = HASS_TOPIC_PREFIX;
    topic += particleDeviceName;
    topic += "/";
    topic += String(addr);
    topic += HASS_TOPIC_STATE_SUFFIX;
    clientHass.publish(topic, buffer, true);
    debug("exit sendStateHass %d", addr);
}

void updateLamp(uint8_t addr)
{
    debug("Update %d", addr);
    moodlamp_t &lamp = deviceList[addr];
    setColor(addr, lamp.rgb.nscale8_video(lamp.isOn ? lamp.brightness : 0));
    sendStateHass(addr);
}

// Callback signature for MQTT subscriptions
void callbackHass(char* topic, uint8_t* payload, unsigned int length) {
    // check for topic prefix and consume it
    String prefix = HASS_TOPIC_PREFIX;
    prefix += particleDeviceName;
    prefix += "/";

    if (strncmp(topic, prefix.c_str(), prefix.length()) != 0) {
        return;
    }
    topic += prefix.length();

    // find referenced moodlamp
    uint8_t addr = 0;
    for (unsigned char i = 2; i < MAX_DEVICES; ++i) {
        if ((String(i) + HASS_TOPIC_SET_SUFFIX).equals(topic)) {
            addr = i;
            break;
        }
    }
    if (!addr) {
        return;
    }

    // parse the command payload
    StaticJsonBuffer<250> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *)payload);

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

void sendDiscoveryToken(uint8_t addr, bool remove = false) {
    debug("enter sendDiscoveryToken %d", addr);
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    String topic = HASS_TOPIC_PREFIX;
    topic += particleDeviceName;
    topic += "/";
    topic += String(addr);
    char buffer[300] = "";
    if (!remove) {
        root["~"] = topic.c_str();
        root["name"] = deviceList[addr].id.c_str(),
        root["unique_id"] = deviceList[addr].id.c_str(),
        root["cmd_t"] = "~" HASS_TOPIC_SET_SUFFIX;
        root["stat_t"] = "~" HASS_TOPIC_STATE_SUFFIX;
        root["schema"] = "json";
        root["rgb"] = true;
        root["brightness"] = true;
        root.printTo(buffer, sizeof(buffer));
    }
    topic += HASS_TOPIC_CONFIG_SUFFIX;
    debug(buffer);
    clientHass.publish(topic, buffer, true);
    debug("exit sendDiscoveryToken %d", addr);
}

bool connectHassOnDemand() {
    if (particleDeviceName.length()) {
        if (clientHass.isConnected()) {
            return true;
        }
        clientHass.connect(
            particleDeviceName.c_str(),
            HASS_ACCESS_USER,
            HASS_ACCESS_PASS);
        
        if (clientHass.isConnected()) {
            debug("Connected to HASS");
            return true;
        }
    }
    return false;
}

void loopHASS() {
    if (!connectHassOnDemand()) {
        return;
    }
    unsigned long now = millis();

    for (unsigned char i = 2; i < MAX_DEVICES; ++i) {
        bool alive = !deviceList[i].id.equals("") && (now - deviceList[i].lastSeen) < ALIVE_TIMEOUT;
        mqttState_t state = deviceList[i].mqttState;

        // wait after starting up to give active lamps the chance to send their beacons
        if (!alive && now < STARTUP_TOLERANCE) {
            continue;
        }

        if (state == unknown || alive != (state == registered)) {
            debug("difference for %d", i);
            String topic = HASS_TOPIC_PREFIX;
            topic += particleDeviceName;
            topic += "/";
            topic += String(i);
            topic += HASS_TOPIC_SET_SUFFIX;

            if (alive) {
                clientHass.subscribe(topic);
                sendDiscoveryToken(i);
                deviceList[i].mqttState = registered;
            } else {
                clientHass.unsubscribe(topic);
                sendDiscoveryToken(i, true);
                deviceList[i].mqttState = unregistered;
                deviceList[i].id = "";
            }
        }
    }
    // Loop the MQTT client
    clientHass.loop();
}

// Open a serial terminal and see the device name printed out
void handlerDeviceName(const char *topic, const char *data) {
    if (strcmp(topic, "particle/device/name") == 0) {
        particleDeviceName = data;
        debug(String("Received device name ") + data);
    }
}

void setup() {
    Particle.subscribe("particle/device/name", handlerDeviceName);
    Particle.publish("particle/device/name");
    ubrf_init();
    connectHassOnDemand();
    debug("Initialized");
}

void loop() {
    if (ubrf_getPacket(&packet) == UB_OK) {
        unsigned char len = sizeof(struct ubheader_t) + packet.header.len + 2;
        char b[5];
        String m;
        for (unsigned char i = 0; i < len; ++i) {
        	char x = ((char*)&packet)[i];
        	if (x < 32 || x > 127) {
        		sprintf(b, "\\%02x", x);
                m += b;
        	} else {
        		m += x;
        	}
        }
        debug("read: %s", m.c_str());
        
        if (packet.header.dest == UB_ADDRESS_BROADCAST || packet.header.dest == UB_ADDRESS_MASTER) {
            unsigned char addr = packet.header.src;
            switch (packet.data[0]) {
                // MGT_DISCOVER is sent, if the lamp has no assigned address yet
                case MGT_DISCOVER:
                    // null-terminate id string
                    packet.data[packet.header.len] = 0;
                    // it's a lamp without address, so assign one
                    if (addr = addDevice(packet.data + 7)) {
                        assignAddr(packet.data + 7, addr);
                        // send default rgb values to lamp and MQTT server
                        updateLamp(addr);
                    }
                    break;
                // MGT_IDENTIFY is sent, if the lamp has an address but is not bound to a master
                case MGT_IDENTIFY:
                    // null-terminate id string
                    packet.data[packet.header.len] = 0;
                    if (!isKnownDevice(packet.data + 1, addr)) {
                        if (addr = addDevice(packet.data + 1)) {
                            // if we haven't had the lamp in our records, we need to assign a new address
                            assignAddr(packet.data + 1, addr);
                            // send default rgb values to lamp and MQTT server
                            updateLamp(addr);
                        }
                    }
                    break;
                    // We could connect the lamp now, but then it would no longer reveal its ID.
                    // So, if the bridge restarts, all connected lamps would keep their address and the bridge wouldn't know their IDs.
                    // That could lead to address clashes. So we simply let them repeat their names after we set their addresses.
                    //
					packet.header.dest = packet.header.src;
					packet.header.src = UB_ADDRESS_MASTER;
					packet.header.flags = UB_PACKET_MGT | UB_PACKET_NOACK;
					packet.header.cls = UB_CLASS_MOODLAMP;
					packet.header.len = 1;
					packet.data[0] = 'O';
					if (ubrf_sendPacket(&packet) == UB_ERROR) {
                        debug("Couldn't send BIND packet");
                    }
					break;
                // MGT_ALIVE is sent, if the lamp is bound to a master
                case MGT_ALIVE:
                    // setColor(addr, rgb_t(rand() & 0xff, rand() & 0xff, rand() & 0xff));
                    break;
            }
            if (addr < MAX_DEVICES) {
                deviceList[addr].lastSeen = millis();
            }
        }    
    }
    loopHASS();
    ubrf12_rxstart();

}
