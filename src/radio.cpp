#include "radio.h"
#include <SPI.h>
#include <esp_mac.h>

// ISR flag for receive interrupt
static volatile bool _rxFlag = false;

#if defined(ESP32)
static void IRAM_ATTR _rxISR(void) {
#else
static void _rxISR(void) {
#endif
    _rxFlag = true;
}

Radio::Radio() {
    _radio = nullptr;
    _initialized = false;
    _freq = LORA_FREQ;
    _power = LORA_POWER;
    _packetCount = 0;
    _nodeId = 0;
}

bool Radio::init() {
    // Create module using the shared SPI bus
    // SPI must already be initialized with correct pins before calling this
    Module* mod = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                             RADIO_RST_PIN, RADIO_BUSY_PIN, SPI);
    _radio = new SX1262(mod);

    // Initialize with Meshtastic-compatible settings
    int state = _radio->begin(
        _freq,          // frequency
        LORA_BW,        // bandwidth
        LORA_SF,        // spreading factor
        LORA_CR,        // coding rate
        LORA_SYNC,      // sync word
        _power,         // output power
        LORA_PREAMBLE   // preamble length
    );

    if (state != RADIOLIB_ERR_NONE) {
        return false;
    }

    // Meshtastic compatibility: disable hardware CRC, use software CRC
    _radio->setCRC(0);
    _radio->setCurrentLimit(140.0);
    _radio->setDio2AsRfSwitch(true);

    // Generate unique node ID from ESP32 MAC address
    _generateNodeId();

    // Set up DIO1 as receive interrupt
    _radio->setDio1Action(_rxISR);

    _initialized = true;
    startReceive();
    return true;
}

void Radio::_generateNodeId() {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    _nodeId = ((uint32_t)mac[2] << 24) |
              ((uint32_t)mac[3] << 16) |
              ((uint32_t)mac[4] << 8)  |
              (uint32_t)mac[5];
}

uint32_t Radio::_nextPacketId() {
    return ++_packetCount;
}

bool Radio::send(const uint8_t* data, size_t len) {
    if (!_initialized) return false;

    int state = _radio->transmit((uint8_t*)data, len);

    // Always restart receive after transmit
    startReceive();

    return (state == RADIOLIB_ERR_NONE);
}

bool Radio::sendMeshPacket(MeshPacket& packet) {
    if (!_initialized) return false;

    // Build raw packet: 16-byte header + payload
    uint8_t raw[MESH_HEADER_LEN + MESH_MAX_PAYLOAD];
    size_t idx = 0;

    // Destination (4 bytes little-endian)
    raw[idx++] = (packet.dest >>  0) & 0xFF;
    raw[idx++] = (packet.dest >>  8) & 0xFF;
    raw[idx++] = (packet.dest >> 16) & 0xFF;
    raw[idx++] = (packet.dest >> 24) & 0xFF;

    // Source (4 bytes little-endian)
    raw[idx++] = (packet.source >>  0) & 0xFF;
    raw[idx++] = (packet.source >>  8) & 0xFF;
    raw[idx++] = (packet.source >> 16) & 0xFF;
    raw[idx++] = (packet.source >> 24) & 0xFF;

    // Packet ID (4 bytes little-endian)
    raw[idx++] = (packet.packetId >>  0) & 0xFF;
    raw[idx++] = (packet.packetId >>  8) & 0xFF;
    raw[idx++] = (packet.packetId >> 16) & 0xFF;
    raw[idx++] = (packet.packetId >> 24) & 0xFF;

    // Flags + channel hash + 2 reserved bytes
    raw[idx++] = packet.flags;
    raw[idx++] = packet.channelHash;
    raw[idx++] = 0;  // reserved
    raw[idx++] = 0;  // reserved

    // Payload
    if (packet.payloadLen > MESH_MAX_PAYLOAD) {
        packet.payloadLen = MESH_MAX_PAYLOAD;
    }
    memcpy(&raw[idx], packet.payload, packet.payloadLen);
    idx += packet.payloadLen;

    return send(raw, idx);
}

bool Radio::hasPacket() {
    return _rxFlag;
}

bool Radio::receive(uint8_t* data, size_t& len, int16_t& rssi, float& snr) {
    if (!_initialized || !_rxFlag) return false;

    _rxFlag = false;

    int length = _radio->getPacketLength();
    if (length <= 0) {
        startReceive();
        return false;
    }

    int state = _radio->readData(data, length);

    if (state == RADIOLIB_ERR_NONE) {
        len  = (size_t)length;
        rssi = _radio->getRSSI();
        snr  = _radio->getSNR();
        startReceive();
        return true;
    }

    startReceive();
    return false;
}

bool Radio::receiveMeshPacket(MeshPacket& packet) {
    uint8_t raw[MESH_HEADER_LEN + MESH_MAX_PAYLOAD];
    size_t len;
    int16_t rssi;
    float snr;

    if (!receive(raw, len, rssi, snr)) return false;
    if (len < MESH_HEADER_LEN) return false;

    // Parse header
    size_t idx = 0;

    packet.dest = (uint32_t)raw[idx]        |
                  ((uint32_t)raw[idx+1] << 8)  |
                  ((uint32_t)raw[idx+2] << 16) |
                  ((uint32_t)raw[idx+3] << 24);
    idx += 4;

    packet.source = (uint32_t)raw[idx]        |
                    ((uint32_t)raw[idx+1] << 8)  |
                    ((uint32_t)raw[idx+2] << 16) |
                    ((uint32_t)raw[idx+3] << 24);
    idx += 4;

    packet.packetId = (uint32_t)raw[idx]        |
                      ((uint32_t)raw[idx+1] << 8)  |
                      ((uint32_t)raw[idx+2] << 16) |
                      ((uint32_t)raw[idx+3] << 24);
    idx += 4;

    packet.flags       = raw[idx++];
    packet.channelHash = raw[idx++];
    idx += 2;  // skip reserved

    // Payload
    packet.payloadLen = len - idx;
    if (packet.payloadLen > MESH_MAX_PAYLOAD) {
        packet.payloadLen = MESH_MAX_PAYLOAD;
    }
    memcpy(packet.payload, &raw[idx], packet.payloadLen);

    packet.rssi = rssi;
    packet.snr  = snr;

    return true;
}

void Radio::startReceive() {
    if (_initialized) {
        _radio->startReceive();
    }
}

void Radio::setFrequency(float freq) {
    _freq = freq;
    if (_initialized) {
        _radio->setFrequency(freq);
        startReceive();
    }
}

void Radio::setPower(int8_t power) {
    _power = power;
    if (_initialized) {
        _radio->setOutputPower(power);
    }
}
