#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <RadioLib.h>
#include "config.h"

// Meshtastic-compatible packet header
struct MeshPacket {
    uint32_t dest;
    uint32_t source;
    uint32_t packetId;
    uint8_t  flags;         // hop_limit:3, want_ack:1, via_mqtt:1, hop_start:3
    uint8_t  channelHash;
    uint8_t  payload[MESH_MAX_PAYLOAD];
    uint8_t  payloadLen;
    int16_t  rssi;
    float    snr;
};

class Radio {
public:
    Radio();

    bool init();
    bool send(const uint8_t* data, size_t len);
    bool sendMeshPacket(MeshPacket& packet);
    bool receive(uint8_t* data, size_t& len, int16_t& rssi, float& snr);
    bool receiveMeshPacket(MeshPacket& packet);
    bool hasPacket();
    void startReceive();

    // Configuration
    void setFrequency(float freq);
    void setPower(int8_t power);

    // Getters
    float    getFrequency() const { return _freq; }
    int8_t   getPower()     const { return _power; }
    bool     isInitialized() const { return _initialized; }
    uint32_t getNodeId()    const { return _nodeId; }

private:
    SX1262*  _radio;
    bool     _initialized;
    float    _freq;
    int8_t   _power;
    uint32_t _nodeId;
    uint32_t _packetCount;

    void     _generateNodeId();
    uint32_t _nextPacketId();
};

#endif // RADIO_H
