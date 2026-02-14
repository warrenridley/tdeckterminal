#ifndef MESHTASTIC_CLI_H
#define MESHTASTIC_CLI_H

#include <Arduino.h>
#include "terminal.h"
#include "radio.h"
#include "config.h"

struct MeshNode {
    uint32_t      nodeId;
    char          shortName[5];
    char          longName[40];
    int16_t       lastRssi;
    float         lastSnr;
    unsigned long lastSeen;
    uint8_t       hopCount;
    bool          active;
};

class MeshtasticCLI {
public:
    MeshtasticCLI(Terminal& term, Radio& radio);

    void init();
    bool handleCommand(const char* cmd, const char* args);
    void checkIncoming();
    void printHelp();

    // Commands
    void cmdSend(const char* args);
    void cmdDM(const char* args);
    void cmdNodes(const char* args);
    void cmdInfo(const char* args);
    void cmdChannel(const char* args);
    void cmdFreq(const char* args);
    void cmdPower(const char* args);
    void cmdTrace(const char* args);

    int getNodeCount() const { return _nodeCount; }

private:
    Terminal& _term;
    Radio&    _radio;

    MeshNode  _nodes[MAX_NODES];
    int       _nodeCount;
    uint8_t   _channelKey[32];
    char      _channelName[16];

    void    _addOrUpdateNode(uint32_t id, int16_t rssi, float snr);
    int     _findNode(uint32_t id);
    void    _processTextMessage(MeshPacket& pkt);
    String  _nodeIdStr(uint32_t id);
};

#endif // MESHTASTIC_CLI_H
