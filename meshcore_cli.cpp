#include "meshcore_cli.h"
#include <string.h>

MeshCoreCLI::MeshCoreCLI(Terminal& term, Radio& radio)
    : _term(term), _radio(radio) {
    _nodeCount    = 0;
    _repeaterMode = false;
    _maxHops      = 3;
}

void MeshCoreCLI::init() {
    // MeshCore protocol initialization
}

bool MeshCoreCLI::handleCommand(const char* cmd, const char* args) {
    if (strcmp(cmd, "send") == 0 || strcmp(cmd, "msg") == 0)   { cmdSend(args);     return true; }
    if (strcmp(cmd, "dm") == 0)                                 { cmdDM(args);       return true; }
    if (strcmp(cmd, "nodes") == 0 || strcmp(cmd, "peers") == 0) { cmdNodes(args);    return true; }
    if (strcmp(cmd, "info") == 0)                                { cmdInfo(args);     return true; }
    if (strcmp(cmd, "ping") == 0)                                { cmdPing(args);     return true; }
    if (strcmp(cmd, "route") == 0)                               { cmdRoute(args);    return true; }
    if (strcmp(cmd, "repeater") == 0 || strcmp(cmd, "repeat") == 0) { cmdRepeater(args); return true; }
    if (strcmp(cmd, "config") == 0)                              { cmdConfig(args);   return true; }
    return false;
}

void MeshCoreCLI::printHelp() {
    _term.printlnColored("MeshCore Commands:", COLOR_HEADER);
    _term.printlnColored("  core send <msg>        Broadcast message", COLOR_INFO);
    _term.printlnColored("  core dm <id> <msg>     Direct message", COLOR_INFO);
    _term.printlnColored("  core nodes             List known nodes", COLOR_INFO);
    _term.printlnColored("  core info              Protocol & radio info", COLOR_INFO);
    _term.printlnColored("  core ping <id>         Ping a node", COLOR_INFO);
    _term.printlnColored("  core route <id>        Request route to node", COLOR_INFO);
    _term.printlnColored("  core repeater [on|off] Toggle repeater mode", COLOR_INFO);
    _term.printlnColored("  core config            Show configuration", COLOR_INFO);
}

// ---- core send ----

void MeshCoreCLI::cmdSend(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: core send <message>", COLOR_ERROR);
        return;
    }
    if (!_radio.isInitialized()) {
        _term.printlnColored("Error: Radio not initialized", COLOR_ERROR);
        return;
    }

    MCPacket pkt;
    pkt.type     = MC_TYPE_TEXT;
    pkt.source   = _radio.getNodeId();
    pkt.dest     = 0xFFFFFFFF;  // Broadcast
    pkt.packetId = millis() & 0xFFFFFFFF;
    pkt.hops     = 0;
    pkt.maxHops  = _maxHops;

    size_t msgLen = strlen(args);
    if (msgLen > MC_MAX_PAYLOAD) msgLen = MC_MAX_PAYLOAD;
    memcpy(pkt.payload, args, msgLen);
    pkt.payloadLen = (uint8_t)msgLen;

    if (_sendPacket(pkt)) {
        _term.printColored("[MC TX] ", COLOR_DIM);
        _term.printColored(">> ", COLOR_PROMPT);
        _term.println(args);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- core dm ----

void MeshCoreCLI::cmdDM(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: core dm <node_id> <message>", COLOR_ERROR);
        return;
    }

    char idStr[16];
    int i = 0;
    while (args[i] && args[i] != ' ' && i < 15) {
        idStr[i] = args[i]; i++;
    }
    idStr[i] = '\0';

    const char* msg = (args[i] == ' ') ? &args[i + 1] : nullptr;
    if (!msg || strlen(msg) == 0) {
        _term.printlnColored("Usage: core dm <node_id> <message>", COLOR_ERROR);
        return;
    }

    uint32_t destId = strtoul(idStr, NULL, 16);

    MCPacket pkt;
    pkt.type     = MC_TYPE_TEXT;
    pkt.source   = _radio.getNodeId();
    pkt.dest     = destId;
    pkt.packetId = millis() & 0xFFFFFFFF;
    pkt.hops     = 0;
    pkt.maxHops  = _maxHops;

    size_t msgLen = strlen(msg);
    if (msgLen > MC_MAX_PAYLOAD) msgLen = MC_MAX_PAYLOAD;
    memcpy(pkt.payload, msg, msgLen);
    pkt.payloadLen = (uint8_t)msgLen;

    if (_sendPacket(pkt)) {
        _term.printf("[MC DM->%08X] ", destId);
        _term.printColored(">> ", COLOR_PROMPT);
        _term.println(msg);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- core nodes ----

void MeshCoreCLI::cmdNodes(const char* args) {
    if (_nodeCount == 0) {
        _term.printlnColored("No MeshCore nodes discovered.", COLOR_WARNING);
        return;
    }

    _term.printlnColored("  Node ID     Name              Hops  RSSI   SNR   Seen", COLOR_HEADER);
    _term.printlnColored("  --------    ----              ----  ----   ---   ----", COLOR_HEADER);

    for (int i = 0; i < _nodeCount; i++) {
        if (!_nodes[i].active) continue;
        unsigned long ago = (millis() - _nodes[i].lastSeen) / 1000;

        char line[80];
        snprintf(line, sizeof(line), "  %08X    %-18s %2d   %4ddBm %4.1f  %lus",
                 _nodes[i].nodeId,
                 strlen(_nodes[i].name) > 0 ? _nodes[i].name : "Unknown",
                 _nodes[i].hops,
                 _nodes[i].lastRssi,
                 _nodes[i].lastSnr,
                 ago);
        _term.println(line);
    }
    _term.printf("\nTotal: %d node(s)\n", _nodeCount);
}

// ---- core info ----

void MeshCoreCLI::cmdInfo(const char* args) {
    _term.printlnColored("=== MeshCore Info ===", COLOR_HEADER);
    _term.printf("Node ID:      %08X\n", _radio.getNodeId());
    _term.printf("Max Hops:     %d\n", _maxHops);
    _term.printf("Repeater:     %s\n", _repeaterMode ? "ON" : "OFF");
    _term.printf("Nodes seen:   %d\n", _nodeCount);
    _term.printf("Frequency:    %.3f MHz\n", _radio.getFrequency());
    _term.printf("TX Power:     %d dBm\n", _radio.getPower());
}

// ---- core ping ----

void MeshCoreCLI::cmdPing(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: core ping <node_id>", COLOR_ERROR);
        return;
    }

    uint32_t destId = strtoul(args, NULL, 16);

    MCPacket pkt;
    pkt.type     = MC_TYPE_PING;
    pkt.source   = _radio.getNodeId();
    pkt.dest     = destId;
    pkt.packetId = millis() & 0xFFFFFFFF;
    pkt.hops     = 0;
    pkt.maxHops  = _maxHops;
    pkt.payloadLen = 0;

    if (_sendPacket(pkt)) {
        _term.printf("PING %08X ...\n", destId);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- core route ----

void MeshCoreCLI::cmdRoute(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: core route <node_id>", COLOR_ERROR);
        return;
    }

    uint32_t destId = strtoul(args, NULL, 16);

    MCPacket pkt;
    pkt.type     = MC_TYPE_ROUTE_REQ;
    pkt.source   = _radio.getNodeId();
    pkt.dest     = destId;
    pkt.packetId = millis() & 0xFFFFFFFF;
    pkt.hops     = 0;
    pkt.maxHops  = _maxHops;
    pkt.payloadLen = 0;

    if (_sendPacket(pkt)) {
        _term.printf("Route request sent to %08X\n", destId);
    } else {
        _term.printlnColored("Error: TX failed", COLOR_ERROR);
    }
}

// ---- core repeater ----

void MeshCoreCLI::cmdRepeater(const char* args) {
    if (args && strlen(args) > 0) {
        if (strcmp(args, "on") == 0) {
            _repeaterMode = true;
            _term.printlnColored("Repeater mode: ON", COLOR_FG);
        } else if (strcmp(args, "off") == 0) {
            _repeaterMode = false;
            _term.printlnColored("Repeater mode: OFF", COLOR_FG);
        } else {
            _term.printlnColored("Usage: core repeater [on|off]", COLOR_ERROR);
        }
    } else {
        _term.printf("Repeater mode: %s\n", _repeaterMode ? "ON" : "OFF");
    }
}

// ---- core config ----

void MeshCoreCLI::cmdConfig(const char* args) {
    _term.printlnColored("=== MeshCore Configuration ===", COLOR_HEADER);
    _term.printf("Max hops:     %d\n", _maxHops);
    _term.printf("Repeater:     %s\n", _repeaterMode ? "ON" : "OFF");
    _term.printf("Node count:   %d / %d\n", _nodeCount, MAX_NODES);
    _term.printf("Frequency:    %.3f MHz\n", _radio.getFrequency());
    _term.printf("TX Power:     %d dBm\n", _radio.getPower());
}

// ---- Packet TX/RX ----

bool MeshCoreCLI::_sendPacket(MCPacket& pkt) {
    uint8_t raw[MC_HEADER_LEN + MC_MAX_PAYLOAD];
    size_t idx = 0;

    raw[idx++] = pkt.type;

    raw[idx++] = (pkt.source >>  0) & 0xFF;
    raw[idx++] = (pkt.source >>  8) & 0xFF;
    raw[idx++] = (pkt.source >> 16) & 0xFF;
    raw[idx++] = (pkt.source >> 24) & 0xFF;

    raw[idx++] = (pkt.dest >>  0) & 0xFF;
    raw[idx++] = (pkt.dest >>  8) & 0xFF;
    raw[idx++] = (pkt.dest >> 16) & 0xFF;
    raw[idx++] = (pkt.dest >> 24) & 0xFF;

    raw[idx++] = (pkt.packetId >>  0) & 0xFF;
    raw[idx++] = (pkt.packetId >>  8) & 0xFF;
    raw[idx++] = (pkt.packetId >> 16) & 0xFF;
    raw[idx++] = (pkt.packetId >> 24) & 0xFF;

    raw[idx++] = pkt.hops;
    raw[idx++] = pkt.maxHops;
    raw[idx++] = pkt.payloadLen;

    memcpy(&raw[idx], pkt.payload, pkt.payloadLen);
    idx += pkt.payloadLen;

    return _radio.send(raw, idx);
}

bool MeshCoreCLI::_receivePacket(MCPacket& pkt) {
    uint8_t raw[MC_HEADER_LEN + MC_MAX_PAYLOAD];
    size_t len;
    int16_t rssi;
    float snr;

    if (!_radio.receive(raw, len, rssi, snr)) return false;
    if (len < MC_HEADER_LEN) return false;

    size_t idx = 0;
    pkt.type = raw[idx++];

    pkt.source = (uint32_t)raw[idx] | ((uint32_t)raw[idx+1] << 8) |
                 ((uint32_t)raw[idx+2] << 16) | ((uint32_t)raw[idx+3] << 24);
    idx += 4;

    pkt.dest = (uint32_t)raw[idx] | ((uint32_t)raw[idx+1] << 8) |
               ((uint32_t)raw[idx+2] << 16) | ((uint32_t)raw[idx+3] << 24);
    idx += 4;

    pkt.packetId = (uint32_t)raw[idx] | ((uint32_t)raw[idx+1] << 8) |
                   ((uint32_t)raw[idx+2] << 16) | ((uint32_t)raw[idx+3] << 24);
    idx += 4;

    pkt.hops       = raw[idx++];
    pkt.maxHops    = raw[idx++];
    pkt.payloadLen = raw[idx++];

    if (pkt.payloadLen > MC_MAX_PAYLOAD) pkt.payloadLen = MC_MAX_PAYLOAD;
    memcpy(pkt.payload, &raw[idx], pkt.payloadLen);

    pkt.rssi = rssi;
    pkt.snr  = snr;

    return true;
}

void MeshCoreCLI::checkIncoming() {
    MCPacket pkt;
    if (_receivePacket(pkt)) {
        _addOrUpdateNode(pkt.source, pkt.rssi, pkt.snr, pkt.hops);
        _processIncoming(pkt);
    }
}

void MeshCoreCLI::_processIncoming(MCPacket& pkt) {
    switch (pkt.type) {

    case MC_TYPE_TEXT: {
        char msg[MC_MAX_PAYLOAD + 1];
        memcpy(msg, pkt.payload, pkt.payloadLen);
        msg[pkt.payloadLen] = '\0';

        _term.println();
        if (pkt.dest == 0xFFFFFFFF) {
            _term.printf("[MC RX %08X] ", pkt.source);
        } else {
            _term.printf("[MC DM %08X] ", pkt.source);
        }
        _term.printColored("<< ", COLOR_WARNING);
        _term.print(msg);
        _term.printf(" (RSSI:%d SNR:%.1f Hops:%d)\n", pkt.rssi, pkt.snr, pkt.hops);
        break;
    }

    case MC_TYPE_PING: {
        _term.printf("\n[MC PING from %08X]", pkt.source);
        // Auto-reply with PONG
        MCPacket reply;
        reply.type       = MC_TYPE_PONG;
        reply.source     = _radio.getNodeId();
        reply.dest       = pkt.source;
        reply.packetId   = millis() & 0xFFFFFFFF;
        reply.hops       = 0;
        reply.maxHops    = _maxHops;
        reply.payloadLen = 0;
        _sendPacket(reply);
        _term.printlnColored(" PONG sent", COLOR_INFO);
        break;
    }

    case MC_TYPE_PONG:
        _term.printf("\n[MC PONG from %08X] RSSI:%ddBm SNR:%.1f Hops:%d\n",
                     pkt.source, pkt.rssi, pkt.snr, pkt.hops);
        break;

    case MC_TYPE_ROUTE_REP:
        _term.printf("\n[MC ROUTE from %08X] Hops:%d RSSI:%ddBm\n",
                     pkt.source, pkt.hops, pkt.rssi);
        break;

    default:
        break;
    }

    // Repeater: forward if enabled and within hop budget
    if (_repeaterMode && pkt.hops < pkt.maxHops &&
        pkt.dest != _radio.getNodeId() && pkt.source != _radio.getNodeId()) {
        pkt.hops++;
        _sendPacket(pkt);
    }
}

// ---- Node management ----

void MeshCoreCLI::_addOrUpdateNode(uint32_t id, int16_t rssi, float snr, uint8_t hops) {
    int idx = _findNode(id);
    if (idx >= 0) {
        _nodes[idx].lastRssi = rssi;
        _nodes[idx].lastSnr  = snr;
        _nodes[idx].lastSeen = millis();
        _nodes[idx].hops     = hops;
    } else if (_nodeCount < MAX_NODES) {
        idx = _nodeCount++;
        memset(&_nodes[idx], 0, sizeof(MCNode));
        _nodes[idx].nodeId   = id;
        _nodes[idx].lastRssi = rssi;
        _nodes[idx].lastSnr  = snr;
        _nodes[idx].lastSeen = millis();
        _nodes[idx].hops     = hops;
        _nodes[idx].active   = true;
    }
}

int MeshCoreCLI::_findNode(uint32_t id) {
    for (int i = 0; i < _nodeCount; i++) {
        if (_nodes[i].nodeId == id) return i;
    }
    return -1;
}
