#ifndef SHELL_H
#define SHELL_H

#include <Arduino.h>
#include "terminal.h"
#include "keyboard.h"
#include "radio.h"
#include "meshtastic_cli.h"
#include "meshcore_cli.h"
#include "config.h"

class Shell {
public:
    Shell(Terminal& term, Keyboard& kb, Radio& radio,
          MeshtasticCLI& meshCli, MeshCoreCLI& coreCli);

    void init();
    void update();          // Call every loop iteration
    void showPrompt();
    void processCommand(const char* cmdLine);

private:
    Terminal&       _term;
    Keyboard&       _kb;
    Radio&          _radio;
    MeshtasticCLI&  _meshCli;
    MeshCoreCLI&    _coreCli;

    char _cmdBuf[CMD_MAX_LEN];
    int  _cmdLen;

    char _history[CMD_HISTORY_SIZE][CMD_MAX_LEN];
    int  _historyCount;
    int  _historyIdx;

    unsigned long _bootTime;

    // Input handling
    void _handleChar(char c);
    void _execute();
    void _addHistory(const char* cmd);
    void _historyUp();
    void _historyDown();
    void _clearInputLine();

    // Built-in commands
    void _cmdHelp(const char* args);
    void _cmdClear(const char* args);
    void _cmdReboot(const char* args);
    void _cmdUptime(const char* args);
    void _cmdFree(const char* args);
    void _cmdUname(const char* args);
    void _cmdEcho(const char* args);
    void _cmdDate(const char* args);
    void _cmdBattery(const char* args);
    void _cmdIfconfig(const char* args);
    void _cmdPs(const char* args);
    void _cmdDmesg(const char* args);
    void _cmdLs(const char* args);
    void _cmdCat(const char* args);
    void _cmdHistory(const char* args);
    void _cmdNeofetch(const char* args);
    void _cmdScan(const char* args);
    void _cmdMonitor(const char* args);
    void _cmdWhoami(const char* args);
    void _cmdHostname(const char* args);
    void _cmdPwd(const char* args);
};

#endif // SHELL_H
