#include "meshtastic_cli.h"
#include <string.h>

// Meshtastic portnum constants
#define PORTNUM_TEXT_MESSAGE    1
#define PORTNUM_NODEINFO        4
#define PORTNUM_POSITION        3
#define PORTNUM_TELEMETRY       67
#define PORTNUM_TRACEROUTE      70

MeshtasticCLI::MeshtasticCLI(Terminal& term, Radio& radio)
    : _term(term), _radio(radio) {
    _nodeCount = 0;
    memset(_channelKey, 0, sizeof(_channelKey));
    strncpy(_channelName, "LongFast", sizeof(_channelName));
}

void MeshtasticCLI::init() {
    // Default Meshtastic LongFast key: AQ== (base64) = 0x01
    _channelKey[0] = 0x01;
}

bool MeshtasticCLI::handleCommand(const char* cmd, const char* args) {
    if (strcmp(cmd, "send") == 0 || strcmp(cmd, "msg") == 0)   { cmdSend(args);    return true; }
    if (strcmp(cmd, "dm") == 0)                                 { cmdDM(args);      return true; }
    if (strcmp(cmd, "nodes") == 0 || strcmp(cmd, "peers") == 0) { cmdNodes(args);   return true; }
    if (strcmp(cmd, "info") == 0)                                { cmdInfo(args);    return true; }
    if (strcmp(cmd, "channel") == 0 || strcmp(cmd, "ch") == 0)  { cmdChannel(args); return true; }
    if (strcmp(cmd, "freq") == 0)                                { cmdFreq(args);    return true; }
    if (strcmp(cmd, "power") == 0 || strcmp(cmd, "txpower") == 0) { cmdPower(args); return true; }
    if (strcmp(cmd, "trace") == 0 || strcmp(cmd, "traceroute") == 0) { cmdTrace(args); return true; }
    return false;
}

void MeshtasticCLI::printHelp() {
    _term.printlnColored("Meshtastic Commands:", COLOR_HEADER);
    _term.printlnColored("  mesh send <msg>       Broadcast text message", COLOR_INFO);
    _term.printlnColored("  mesh dm <id> <msg>    Direct message to node", COLOR_INFO);
    _term.printlnColored("  mesh nodes            List discovered nodes", COLOR_INFO);
    _term.printlnColored("  mesh info             Radio & protocol info", COLOR_INFO);
    _term.printlnColored("  mesh channel [name]   Show/set channel name", COLOR_INFO);
    _term.printlnColored("  mesh freq [mhz]       Show/set frequency", COLOR_INFO);
    _term.printlnColored("  mesh power [dbm]      Show/set TX power", COLOR_INFO);
    _term.printlnColored("  mesh trace <id>       Traceroute to node", COLOR_INFO);
}

// ---- mesh send ----

void MeshtasticCLI::cmdSend(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: mesh send <message>", COLOR_ERROR);
        return;
    }
    if (!_radio.isInitialized()) {
        _term.printlnColored("Error: Radio not initialized", COLOR_ERROR);
        return;
    }

    MeshPacket pkt;
    pkt.dest       = MESH_BROADCAST;
    pkt.source     = _radio.getNodeId();
    pkt.packetId   = millis() & 0xFFFFFFFF;
    pkt.flags      = 0x03;   // hop_limit=3
    pkt.channelHash = 0x08;  // LongFast default hash

    // Build simple text payload
    // In production this would be protobuf-encoded + encrypted
    size_t msgLen = strlen(args);
    if (msgLen > MESH_MAX_PAYLOAD - 4) msgLen = MESH_MAX_PAYLOAD - 4;

    pkt.payload[0] = 0x08;              // protobuf: field 1 varint
    pkt.payload[1] = PORTNUM_TEXT_MESSAGE;
    pkt.payload[2] = 0x12;              // protobuf: field 2 length-delimited
    pkt.payload[3] = (uint8_t)msgLen;
    memcpy(&pkt.payload[4], args, msgLen);
    pkt.payloadLen = msgLen + 4;

    if (_radio.sendMeshPacket(pkt)) {
        _term.printColored("[TX] ", COLOR_DIM);
        _term.printColored(">> ", COLOR_PROMPT);
        _term.println(args);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- mesh dm ----

void MeshtasticCLI::cmdDM(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: mesh dm <node_id> <message>", COLOR_ERROR);
        return;
    }

    // Parse: <hex_id> <message>
    char idStr[16];
    int i = 0;
    while (args[i] && args[i] != ' ' && i < 15) {
        idStr[i] = args[i]; i++;
    }
    idStr[i] = '\0';

    const char* msg = (args[i] == ' ') ? &args[i + 1] : nullptr;
    if (!msg || strlen(msg) == 0) {
        _term.printlnColored("Usage: mesh dm <node_id> <message>", COLOR_ERROR);
        return;
    }

    uint32_t destId = strtoul(idStr, NULL, 16);

    MeshPacket pkt;
    pkt.dest       = destId;
    pkt.source     = _radio.getNodeId();
    pkt.packetId   = millis() & 0xFFFFFFFF;
    pkt.flags      = 0x0B;   // hop_limit=3, want_ack=1
    pkt.channelHash = 0x08;

    size_t msgLen = strlen(msg);
    if (msgLen > MESH_MAX_PAYLOAD - 4) msgLen = MESH_MAX_PAYLOAD - 4;

    pkt.payload[0] = 0x08;
    pkt.payload[1] = PORTNUM_TEXT_MESSAGE;
    pkt.payload[2] = 0x12;
    pkt.payload[3] = (uint8_t)msgLen;
    memcpy(&pkt.payload[4], msg, msgLen);
    pkt.payloadLen = msgLen + 4;

    if (_radio.sendMeshPacket(pkt)) {
        _term.printf("[DM->%s] ", _nodeIdStr(destId).c_str());
        _term.printColored(">> ", COLOR_PROMPT);
        _term.println(msg);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- mesh nodes ----

void MeshtasticCLI::cmdNodes(const char* args) {
    if (_nodeCount == 0) {
        _term.printlnColored("No nodes discovered yet.", COLOR_WARNING);
        return;
    }

    _term.printlnColored("  Node ID      Name              RSSI   SNR   Seen", COLOR_HEADER);
    _term.printlnColored("  ---------    ----              ----   ---   ----", COLOR_HEADER);

    for (int i = 0; i < _nodeCount; i++) {
        if (!_nodes[i].active) continue;
        unsigned long ago = (millis() - _nodes[i].lastSeen) / 1000;

        char line[80];
        snprintf(line, sizeof(line), "  !%08X   %-18s %4ddBm %4.1f  %lus",
                 _nodes[i].nodeId,
                 strlen(_nodes[i].longName) > 0 ? _nodes[i].longName : "Unknown",
                 _nodes[i].lastRssi,
                 _nodes[i].lastSnr,
                 ago);
        _term.println(line);
    }
    _term.printf("\nTotal: %d node(s)\n", _nodeCount);
}

// ---- mesh info ----

void MeshtasticCLI::cmdInfo(const char* args) {
    _term.printlnColored("=== Meshtastic Radio Info ===", COLOR_HEADER);
    _term.printf("Node ID:      !%08X\n", _radio.getNodeId());
    _term.printf("Frequency:    %.3f MHz\n", _radio.getFrequency());
    _term.printf("TX Power:     %d dBm\n", _radio.getPower());
    _term.printf("Bandwidth:    %.0f kHz\n", (float)LORA_BW);
    _term.printf("Spread:       SF%d\n", LORA_SF);
    _term.printf("Coding Rate:  4/%d\n", LORA_CR);
    _term.printf("Channel:      %s\n", _channelName);
    _term.printf("Nodes seen:   %d\n", _nodeCount);
    _term.printf("Radio status: %s\n", _radio.isInitialized() ? "ONLINE" : "OFFLINE");
}

// ---- mesh channel ----

void MeshtasticCLI::cmdChannel(const char* args) {
    if (args && strlen(args) > 0) {
        strncpy(_channelName, args, sizeof(_channelName) - 1);
        _channelName[sizeof(_channelName) - 1] = '\0';
        _term.printf("Channel set to: %s\n", _channelName);
    } else {
        _term.printf("Current channel: %s\n", _channelName);
    }
}

// ---- mesh freq ----

void MeshtasticCLI::cmdFreq(const char* args) {
    if (args && strlen(args) > 0) {
        float freq = atof(args);
        if (freq >= 150.0 && freq <= 960.0) {
            _radio.setFrequency(freq);
            _term.printf("Frequency set to: %.3f MHz\n", freq);
        } else {
            _term.printlnColored("Error: Frequency out of range", COLOR_ERROR);
        }
    } else {
        _term.printf("Current frequency: %.3f MHz\n", _radio.getFrequency());
    }
}

// ---- mesh power ----

void MeshtasticCLI::cmdPower(const char* args) {
    if (args && strlen(args) > 0) {
        int power = atoi(args);
        if (power >= -9 && power <= 22) {
            _radio.setPower((int8_t)power);
            _term.printf("TX power set to: %d dBm\n", power);
        } else {
            _term.printlnColored("Error: Power range is -9 to 22 dBm", COLOR_ERROR);
        }
    } else {
        _term.printf("Current TX power: %d dBm\n", _radio.getPower());
    }
}

// ---- mesh trace ----

void MeshtasticCLI::cmdTrace(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: mesh trace <node_id>", COLOR_ERROR);
        return;
    }

    uint32_t destId = strtoul(args, NULL, 16);
    _term.printf("Traceroute to !%08X ...\n", destId);

    MeshPacket pkt;
    pkt.dest       = destId;
    pkt.source     = _radio.getNodeId();
    pkt.packetId   = millis() & 0xFFFFFFFF;
    pkt.flags      = 0x0B;
    pkt.channelHash = 0x08;

    pkt.payload[0] = 0x08;
    pkt.payload[1] = PORTNUM_TRACEROUTE;
    pkt.payload[2] = 0x12;
    pkt.payload[3] = 0;
    pkt.payloadLen = 4;

    if (_radio.sendMeshPacket(pkt)) {
        _term.printlnColored("Traceroute sent, awaiting reply...", COLOR_INFO);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- Incoming packets ----

void MeshtasticCLI::checkIncoming() {
    MeshPacket pkt;
    if (_radio.receiveMeshPacket(pkt)) {
        _addOrUpdateNode(pkt.source, pkt.rssi, pkt.snr);

        if (pkt.dest == MESH_BROADCAST || pkt.dest == _radio.getNodeId()) {
            _processTextMessage(pkt);
        }
    }
}

void MeshtasticCLI::_processTextMessage(MeshPacket& pkt) {
    if (pkt.payloadLen <= 4) return;

    // Check for text message portnum
    if (pkt.payload[1] == PORTNUM_TEXT_MESSAGE) {
        uint8_t msgLen = pkt.payload[3];
        if (msgLen > pkt.payloadLen - 4) msgLen = pkt.payloadLen - 4;

        char msg[MESH_MAX_PAYLOAD];
        memcpy(msg, &pkt.payload[4], msgLen);
        msg[msgLen] = '\0';

        _term.println();
        _term.printf("[RX !%08X] ", pkt.source);
        _term.printColored("<< ", COLOR_WARNING);
        _term.print(msg);
        _term.printf(" (RSSI:%ddBm SNR:%.1f)\n", pkt.rssi, pkt.snr);
    }
}

// ---- Node management ----

void MeshtasticCLI::_addOrUpdateNode(uint32_t id, int16_t rssi, float snr) {
    int idx = _findNode(id);
    if (idx >= 0) {
        _nodes[idx].lastRssi = rssi;
        _nodes[idx].lastSnr  = snr;
        _nodes[idx].lastSeen = millis();
    } else if (_nodeCount < MAX_NODES) {
        idx = _nodeCount++;
        memset(&_nodes[idx], 0, sizeof(MeshNode));
        _nodes[idx].nodeId   = id;
        _nodes[idx].lastRssi = rssi;
        _nodes[idx].lastSnr  = snr;
        _nodes[idx].lastSeen = millis();
        _nodes[idx].active   = true;
    }
}

int MeshtasticCLI::_findNode(uint32_t id) {
    for (int i = 0; i < _nodeCount; i++) {
        if (_nodes[i].nodeId == id) return i;
    }
    return -1;
}

String MeshtasticCLI::_nodeIdStr(uint32_t id) {
    char buf[12];
    snprintf(buf, sizeof(buf), "!%08X", id);
    return String(buf);
}
