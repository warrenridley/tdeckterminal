#ifndef MESHCORE_CLI_H
#define MESHCORE_CLI_H

#include <Arduino.h>
#include "terminal.h"
#include "radio.h"
#include "config.h"

// MeshCore packet types
#define MC_TYPE_TEXT        0x01
#define MC_TYPE_PING        0x02
#define MC_TYPE_PONG        0x03
#define MC_TYPE_ACK         0x04
#define MC_TYPE_NODEINFO    0x05
#define MC_TYPE_ROUTE_REQ   0x06
#define MC_TYPE_ROUTE_REP   0x07
#define MC_TYPE_POSITION    0x08
#define MC_TYPE_TELEMETRY   0x09
#define MC_TYPE_CMD         0x0A

// MeshCore packet header:
// [TYPE:1][SRC:4][DST:4][ID:4][HOPS:1][MAXHOPS:1][LEN:1] = 16 bytes
#define MC_HEADER_LEN       16
#define MC_MAX_PAYLOAD      200

struct MCPacket {
    uint8_t  type;
    uint32_t source;
    uint32_t dest;
    uint32_t packetId;
    uint8_t  hops;
    uint8_t  maxHops;
    uint8_t  payload[MC_MAX_PAYLOAD];
    uint8_t  payloadLen;
    int16_t  rssi;
    float    snr;
};

struct MCNode {
    uint32_t      nodeId;
    char          name[32];
    int16_t       lastRssi;
    float         lastSnr;
    unsigned long lastSeen;
    uint8_t       hops;
    bool          active;
};

class MeshCoreCLI {
public:
    MeshCoreCLI(Terminal& term, Radio& radio);

    void init();
    bool handleCommand(const char* cmd, const char* args);
    void checkIncoming();
    void printHelp();

    // Commands
    void cmdSend(const char* args);
    void cmdDM(const char* args);
    void cmdNodes(const char* args);
    void cmdInfo(const char* args);
    void cmdPing(const char* args);
    void cmdRoute(const char* args);
    void cmdRepeater(const char* args);
    void cmdConfig(const char* args);

    int  getNodeCount() const { return _nodeCount; }
    bool isRepeater()   const { return _repeaterMode; }

private:
    Terminal& _term;
    Radio&    _radio;

    MCNode _nodes[MAX_NODES];
    int    _nodeCount;
    bool   _repeaterMode;
    uint8_t _maxHops;

    void _addOrUpdateNode(uint32_t id, int16_t rssi, float snr, uint8_t hops);
    int  _findNode(uint32_t id);
    bool _sendPacket(MCPacket& pkt);
    bool _receivePacket(MCPacket& pkt);
    void _processIncoming(MCPacket& pkt);
};

#endif // MESHCORE_CLI_H
