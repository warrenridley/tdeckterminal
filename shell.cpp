#include "shell.h"
#include <esp_system.h>
#include <esp_chip_info.h>

Shell::Shell(Terminal& term, Keyboard& kb, Radio& radio,
             MeshtasticCLI& meshCli, MeshCoreCLI& coreCli)
    : _term(term), _kb(kb), _radio(radio),
      _meshCli(meshCli), _coreCli(coreCli) {
    _cmdLen       = 0;
    _historyCount = 0;
    _historyIdx   = -1;
    _bootTime     = 0;
    memset(_cmdBuf, 0, sizeof(_cmdBuf));
}

void Shell::init() {
    _bootTime = millis();
    showPrompt();
}

void Shell::showPrompt() {
    _term.printColored(HOSTNAME, COLOR_PROMPT);
    _term.printColored("@", COLOR_FG);
    _term.printColored(OS_NAME, COLOR_PROMPT);
    _term.printColored(":~$ ", COLOR_FG);
    _cmdLen     = 0;
    _historyIdx = -1;
    memset(_cmdBuf, 0, sizeof(_cmdBuf));
}

void Shell::update() {
    // Keyboard input
    if (_kb.available()) {
        char c = _kb.read();
        if (c != KEY_NONE) {
            _handleChar(c);
        }
    }

    // USB serial input passthrough
    while (Serial.available()) {
        char c = Serial.read();
        if (c != 0) {
            _handleChar(c);
        }
    }

    // Check for incoming radio packets (Meshtastic)
    _meshCli.checkIncoming();

    // Cursor blink
    _term.refresh();
}

// ========== Input Handling ==========

void Shell::_handleChar(char c) {
    switch (c) {
    case KEY_ENTER:
    case '\n':
    case '\r':
        _term.println();
        if (_cmdLen > 0) {
            _cmdBuf[_cmdLen] = '\0';
            _addHistory(_cmdBuf);
            _execute();
        }
        showPrompt();
        break;

    case KEY_BACKSPACE:
    case '\b':
    case 127:
        if (_cmdLen > 0) {
            _cmdLen--;
            _cmdBuf[_cmdLen] = '\0';
            _term.backspace();
        }
        break;

    case KEY_UP:
        _historyUp();
        break;

    case KEY_DOWN:
        _historyDown();
        break;

    case KEY_TAB:
    case '\t':
        // Future: tab completion
        break;

    default:
        if (c >= 32 && c < 127 && _cmdLen < CMD_MAX_LEN - 1) {
            _cmdBuf[_cmdLen++] = c;
            _cmdBuf[_cmdLen] = '\0';
            char s[2] = {c, 0};
            _term.print(s);
        }
        break;
    }
}

void Shell::_execute() {
    char cmd[CMD_MAX_LEN];
    strncpy(cmd, _cmdBuf, CMD_MAX_LEN);

    char* cmdName = strtok(cmd, " ");
    if (!cmdName) return;

    // ---- "mesh" / "meshtastic" prefix ----
    if (strcmp(cmdName, "mesh") == 0 || strcmp(cmdName, "meshtastic") == 0) {
        char* sub = strtok(NULL, " ");
        if (!sub) { _meshCli.printHelp(); return; }
        char* args = strtok(NULL, "");
        if (_meshCli.handleCommand(sub, args)) return;
        _term.printf("%s: unknown mesh command '%s'\n", OS_NAME, sub);
        return;
    }

    // ---- "core" / "meshcore" prefix ----
    if (strcmp(cmdName, "core") == 0 || strcmp(cmdName, "meshcore") == 0) {
        char* sub = strtok(NULL, " ");
        if (!sub) { _coreCli.printHelp(); return; }
        char* args = strtok(NULL, "");
        if (_coreCli.handleCommand(sub, args)) return;
        _term.printf("%s: unknown core command '%s'\n", OS_NAME, sub);
        return;
    }

    // ---- Built-in commands ----
    char* args = strtok(NULL, "");

    if (strcmp(cmdName, "help") == 0)                           { _cmdHelp(args);     return; }
    if (strcmp(cmdName, "clear") == 0 || strcmp(cmdName, "cls") == 0) { _cmdClear(args); return; }
    if (strcmp(cmdName, "reboot") == 0)                         { _cmdReboot(args);   return; }
    if (strcmp(cmdName, "uptime") == 0)                         { _cmdUptime(args);   return; }
    if (strcmp(cmdName, "free") == 0)                            { _cmdFree(args);     return; }
    if (strcmp(cmdName, "uname") == 0)                           { _cmdUname(args);    return; }
    if (strcmp(cmdName, "echo") == 0)                            { _cmdEcho(args);     return; }
    if (strcmp(cmdName, "date") == 0)                            { _cmdDate(args);     return; }
    if (strcmp(cmdName, "battery") == 0 || strcmp(cmdName, "bat") == 0) { _cmdBattery(args); return; }
    if (strcmp(cmdName, "ifconfig") == 0 || strcmp(cmdName, "ip") == 0) { _cmdIfconfig(args); return; }
    if (strcmp(cmdName, "ps") == 0)                              { _cmdPs(args);       return; }
    if (strcmp(cmdName, "dmesg") == 0)                           { _cmdDmesg(args);    return; }
    if (strcmp(cmdName, "ls") == 0)                              { _cmdLs(args);       return; }
    if (strcmp(cmdName, "cat") == 0)                             { _cmdCat(args);      return; }
    if (strcmp(cmdName, "history") == 0)                         { _cmdHistory(args);  return; }
    if (strcmp(cmdName, "neofetch") == 0 || strcmp(cmdName, "fetch") == 0) { _cmdNeofetch(args); return; }
    if (strcmp(cmdName, "scan") == 0)                            { _cmdScan(args);     return; }
    if (strcmp(cmdName, "monitor") == 0 || strcmp(cmdName, "mon") == 0) { _cmdMonitor(args); return; }
    if (strcmp(cmdName, "whoami") == 0)                          { _cmdWhoami(args);   return; }
    if (strcmp(cmdName, "hostname") == 0)                        { _cmdHostname(args); return; }
    if (strcmp(cmdName, "pwd") == 0)                             { _cmdPwd(args);      return; }

    // Unknown command
    _term.printf("-bash: %s: command not found\n", cmdName);
}

// ========== History ==========

void Shell::_addHistory(const char* cmd) {
    if (_historyCount < CMD_HISTORY_SIZE) {
        strncpy(_history[_historyCount], cmd, CMD_MAX_LEN);
        _historyCount++;
    } else {
        for (int i = 0; i < CMD_HISTORY_SIZE - 1; i++) {
            strncpy(_history[i], _history[i + 1], CMD_MAX_LEN);
        }
        strncpy(_history[CMD_HISTORY_SIZE - 1], cmd, CMD_MAX_LEN);
    }
}

void Shell::_historyUp() {
    if (_historyCount == 0) return;

    if (_historyIdx < 0) {
        _historyIdx = _historyCount - 1;
    } else if (_historyIdx > 0) {
        _historyIdx--;
    }

    _clearInputLine();
    strncpy(_cmdBuf, _history[_historyIdx], CMD_MAX_LEN);
    _cmdLen = strlen(_cmdBuf);
    _term.print(_cmdBuf);
}

void Shell::_historyDown() {
    if (_historyIdx < 0) return;

    _historyIdx++;
    _clearInputLine();

    if (_historyIdx >= _historyCount) {
        _historyIdx = -1;
        _cmdLen = 0;
        _cmdBuf[0] = '\0';
    } else {
        strncpy(_cmdBuf, _history[_historyIdx], CMD_MAX_LEN);
        _cmdLen = strlen(_cmdBuf);
        _term.print(_cmdBuf);
    }
}

void Shell::_clearInputLine() {
    // Erase current input by sending backspaces
    for (int i = 0; i < _cmdLen; i++) {
        _term.backspace();
    }
    _cmdLen = 0;
    _cmdBuf[0] = '\0';
}

void Shell::processCommand(const char* cmdLine) {
    strncpy(_cmdBuf, cmdLine, CMD_MAX_LEN - 1);
    _cmdBuf[CMD_MAX_LEN - 1] = '\0';
    _cmdLen = strlen(_cmdBuf);
    _execute();
}

// ========== Built-in Commands ==========

void Shell::_cmdHelp(const char* args) {
    _term.printlnColored("=== " OS_NAME " v" OS_VERSION " ===", COLOR_HEADER);
    _term.println();
    _term.printlnColored("System Commands:", COLOR_HEADER);
    _term.printlnColored("  help            Show this help", COLOR_INFO);
    _term.printlnColored("  clear           Clear terminal", COLOR_INFO);
    _term.printlnColored("  reboot          Reboot device", COLOR_INFO);
    _term.printlnColored("  uptime          Show uptime", COLOR_INFO);
    _term.printlnColored("  free            Memory usage", COLOR_INFO);
    _term.printlnColored("  uname [-a]      System info", COLOR_INFO);
    _term.printlnColored("  echo <text>     Print text", COLOR_INFO);
    _term.printlnColored("  date            Uptime clock", COLOR_INFO);
    _term.printlnColored("  battery         Battery status", COLOR_INFO);
    _term.printlnColored("  ifconfig        Radio interfaces", COLOR_INFO);
    _term.printlnColored("  ps              Running tasks", COLOR_INFO);
    _term.printlnColored("  dmesg           Boot log", COLOR_INFO);
    _term.printlnColored("  ls [path]       List SD files", COLOR_INFO);
    _term.printlnColored("  cat <file>      Read SD file", COLOR_INFO);
    _term.printlnColored("  history         Command history", COLOR_INFO);
    _term.printlnColored("  neofetch        System fetch", COLOR_INFO);
    _term.printlnColored("  scan            RF scan (10s)", COLOR_INFO);
    _term.printlnColored("  monitor         Live packet view", COLOR_INFO);
    _term.printlnColored("  whoami          Current user", COLOR_INFO);
    _term.printlnColored("  hostname        Show hostname", COLOR_INFO);
    _term.printlnColored("  pwd             Working directory", COLOR_INFO);
    _term.println();
    _meshCli.printHelp();
    _term.println();
    _coreCli.printHelp();
}

void Shell::_cmdClear(const char* args) {
    _term.clear();
}

void Shell::_cmdReboot(const char* args) {
    _term.printlnColored("Rebooting...", COLOR_WARNING);
    _term.refresh();
    delay(500);
    esp_restart();
}

void Shell::_cmdUptime(const char* args) {
    unsigned long ms    = millis() - _bootTime;
    unsigned long secs  = ms / 1000;
    unsigned long mins  = secs / 60;
    unsigned long hours = mins / 60;
    unsigned long days  = hours / 24;
    _term.printf(" up %lud %02lu:%02lu:%02lu\n",
                 days, hours % 24, mins % 60, secs % 60);
}

void Shell::_cmdFree(const char* args) {
    _term.printlnColored("              total       used       free", COLOR_HEADER);

    size_t heapTotal = ESP.getHeapSize();
    size_t heapFree  = ESP.getFreeHeap();
    _term.printf("Heap:    %10u %10u %10u\n",
                 heapTotal, heapTotal - heapFree, heapFree);

    size_t psramTotal = ESP.getPsramSize();
    if (psramTotal > 0) {
        size_t psramFree = ESP.getFreePsram();
        _term.printf("PSRAM:   %10u %10u %10u\n",
                     psramTotal, psramTotal - psramFree, psramFree);
    }

    size_t flashTotal = ESP.getFlashChipSize();
    size_t sketchUsed = ESP.getSketchSize();
    _term.printf("Flash:   %10u %10u %10u\n",
                 flashTotal, sketchUsed, flashTotal - sketchUsed);
}

void Shell::_cmdUname(const char* args) {
    if (args && strstr(args, "-a")) {
        _term.printf("%s %s %s ESP32-S3 %s GNU/Mesh\n",
                     OS_NAME, HOSTNAME, KERNEL_VERSION, OS_VERSION);
    } else {
        _term.println(OS_NAME);
    }
}

void Shell::_cmdEcho(const char* args) {
    _term.println(args ? args : "");
}

void Shell::_cmdDate(const char* args) {
    unsigned long secs = millis() / 1000;
    unsigned long h = (secs / 3600) % 24;
    unsigned long m = (secs / 60) % 60;
    unsigned long s = secs % 60;
    _term.printf("Uptime clock: %02lu:%02lu:%02lu (no RTC)\n", h, m, s);
}

void Shell::_cmdBattery(const char* args) {
    int raw = analogRead(BAT_ADC_PIN);
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f;
    int pct = constrain((int)((voltage - 3.0f) / (4.2f - 3.0f) * 100.0f), 0, 100);

    _term.printf("Battery: %.2fV (%d%%)\n", voltage, pct);

    // Draw battery bar
    int filled = pct / 5;  // 0-20 blocks
    _term.print("  [");
    uint16_t barColor = pct > 50 ? COLOR_FG : (pct > 20 ? COLOR_WARNING : COLOR_ERROR);
    for (int i = 0; i < 20; i++) {
        if (i < filled) _term.printColored("#", barColor);
        else            _term.print(" ");
    }
    _term.print("]");

    if (pct > 75)      _term.printlnColored(" Good", COLOR_FG);
    else if (pct > 50) _term.printlnColored(" OK", COLOR_FG);
    else if (pct > 20) _term.printlnColored(" Low", COLOR_WARNING);
    else               _term.printlnColored(" Critical!", COLOR_ERROR);
}

void Shell::_cmdIfconfig(const char* args) {
    _term.printlnColored("lora0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>", COLOR_FG);
    _term.printf("        mesh addr !%08X\n", _radio.getNodeId());
    _term.printf("        freq %.3f MHz  txpower %d dBm\n",
                 _radio.getFrequency(), _radio.getPower());
    _term.printf("        BW %.0f kHz  SF%d  CR 4/%d\n",
                 (float)LORA_BW, LORA_SF, LORA_CR);
    _term.printf("        status %s\n",
                 _radio.isInitialized() ? "UP" : "DOWN");
    _term.println();
    _term.printlnColored("usb0: flags=4099<UP,BROADCAST,MULTICAST>", COLOR_FG);
    _term.println("        link serial USB-CDC 115200");
}

void Shell::_cmdPs(const char* args) {
    _term.printlnColored("  PID  STATE      STACK  NAME", COLOR_HEADER);
    _term.println("    1  running     4096  init");
    _term.println("    2  running     8192  shell");
    _term.println("    3  sleeping    2048  display");
    _term.println("    4  sleeping    1024  keyboard");
    if (_radio.isInitialized()) {
        _term.println("    5  running     4096  radio_rx");
        _term.println("    6  running     2048  meshtastic");
        _term.println("    7  running     2048  meshcore");
    }
    if (_coreCli.isRepeater()) {
        _term.println("    8  running     1024  repeater");
    }
}

void Shell::_cmdDmesg(const char* args) {
    _term.printlnColored("[    0.000] " OS_NAME " " OS_VERSION " booting", COLOR_FG);
    _term.printf("[    0.010] CPU: Xtensa LX7 dual-core @ %dMHz\n", ESP.getCpuFreqMHz());
    _term.printf("[    0.020] Heap: %uK total\n", ESP.getHeapSize() / 1024);
    _term.printf("[    0.030] PSRAM: %uK total\n", ESP.getPsramSize() / 1024);
    _term.printf("[    0.040] Flash: %uMB %s\n",
                 ESP.getFlashChipSize() / (1024 * 1024),
                 ESP.getFlashChipMode() == FM_QIO ? "QIO" : "DIO");
    _term.printlnColored("[    0.100] I2C: bus0 SDA=18 SCL=8 400kHz", COLOR_FG);
    _term.printlnColored("[    0.110] I2C: keyboard at 0x55", COLOR_FG);
    _term.printlnColored("[    0.200] SPI: bus0 MOSI=41 MISO=38 SCK=40", COLOR_FG);
    _term.printlnColored("[    0.210] Display: ST7789V 320x240 rot=1", COLOR_FG);

    if (_radio.isInitialized()) {
        _term.printf("[    0.500] LoRa: SX1262 @ %.3fMHz %ddBm\n",
                     _radio.getFrequency(), _radio.getPower());
        _term.printf("[    0.510] LoRa: node !%08X\n", _radio.getNodeId());
        _term.printlnColored("[    0.520] Meshtastic: protocol ready", COLOR_FG);
        _term.printlnColored("[    0.530] MeshCore: protocol ready", COLOR_FG);
    } else {
        _term.printlnColored("[    0.500] LoRa: SX1262 INIT FAILED", COLOR_ERROR);
    }
    _term.printlnColored("[    1.000] Shell: ready", COLOR_FG);
}

void Shell::_cmdLs(const char* args) {
    _term.printlnColored("total 0", COLOR_FG);
    _term.printlnColored("(SD card not mounted -- insert microSD)", COLOR_WARNING);
}

void Shell::_cmdCat(const char* args) {
    if (!args || strlen(args) == 0) {
        _term.printlnColored("Usage: cat <filename>", COLOR_ERROR);
        return;
    }
    _term.printf("cat: %s: No such file or directory\n", args);
}

void Shell::_cmdHistory(const char* args) {
    for (int i = 0; i < _historyCount; i++) {
        _term.printf("  %3d  %s\n", i + 1, _history[i]);
    }
}

void Shell::_cmdNeofetch(const char* args) {
    _term.printlnColored("        ___           ", COLOR_PROMPT);

    _term.printColored("       /   \\          ", COLOR_PROMPT);
    _term.printColored(HOSTNAME, COLOR_PROMPT);
    _term.printColored("@", COLOR_FG);
    _term.printlnColored(OS_NAME, COLOR_PROMPT);

    _term.printColored("      /mesh \\         ", COLOR_PROMPT);
    _term.printlnColored("----------------", COLOR_FG);

    _term.printColored("     / _  _ \\         ", COLOR_PROMPT);
    _term.printf("OS: %s %s\n", OS_NAME, OS_VERSION);

    _term.printColored("    / | || | \\        ", COLOR_PROMPT);
    _term.printf("Kernel: %s\n", KERNEL_VERSION);

    _term.printColored("   /  |_||_|  \\       ", COLOR_PROMPT);
    _term.println("Platform: ESP32-S3");

    _term.printColored("  /___________\\       ", COLOR_PROMPT);
    _term.printf("CPU: LX7 x2 @ %dMHz\n", ESP.getCpuFreqMHz());

    _term.printColored("  |  T-Deck+  |       ", COLOR_PROMPT);
    _term.printf("Heap: %uK / %uK\n",
                 (ESP.getHeapSize() - ESP.getFreeHeap()) / 1024,
                 ESP.getHeapSize() / 1024);

    if (ESP.getPsramSize() > 0) {
        _term.printColored("  |  /|   |\\  |       ", COLOR_PROMPT);
        _term.printf("PSRAM: %uK / %uK\n",
                     (ESP.getPsramSize() - ESP.getFreePsram()) / 1024,
                     ESP.getPsramSize() / 1024);
    }

    _term.printColored("  |_/ |___| \\_|       ", COLOR_PROMPT);
    _term.printf("Flash: %uMB\n", ESP.getFlashChipSize() / (1024 * 1024));

    _term.print("                      ");
    _term.println("Display: ST7789V 320x240");

    _term.print("                      ");
    _term.printf("Radio: SX1262 %s\n",
                 _radio.isInitialized() ? "ONLINE" : "OFFLINE");

    _term.print("                      ");
    _term.printf("Node: !%08X\n", _radio.getNodeId());

    // Color palette
    _term.print("                      ");
    uint16_t colors[] = {0x0000, 0xF800, 0x07E0, 0xFFE0,
                         0x001F, 0xF81F, 0x07FF, 0xFFFF};
    for (int i = 0; i < 8; i++) {
        _term.printColored("\xDB\xDB", colors[i]);
    }
    _term.println();
}

void Shell::_cmdScan(const char* args) {
    if (!_radio.isInitialized()) {
        _term.printlnColored("Error: Radio not initialized", COLOR_ERROR);
        return;
    }

    _term.printlnColored("Scanning for RF activity (10s)...", COLOR_INFO);
    _term.println();

    int count = 0;
    unsigned long start = millis();

    while (millis() - start < 10000) {
        MeshPacket pkt;
        if (_radio.receiveMeshPacket(pkt)) {
            count++;
            _term.printf("[%3d] src:!%08X dst:!%08X "
                         "rssi:%d snr:%.1f len:%d\n",
                         count, pkt.source, pkt.dest,
                         pkt.rssi, pkt.snr, pkt.payloadLen);
        }
        delay(10);
        _term.refresh();
    }

    _term.printf("\nScan complete: %d packet(s)\n", count);
}

void Shell::_cmdMonitor(const char* args) {
    _term.printlnColored("=== Packet Monitor ===", COLOR_HEADER);
    _term.printlnColored("Press any key to exit.\n", COLOR_WARNING);

    while (!_kb.available() && !Serial.available()) {
        MeshPacket pkt;
        if (_radio.receiveMeshPacket(pkt)) {
            unsigned long ts = millis() / 1000;
            _term.printf("[%lu] %08X->%08X id:%08X "
                         "fl:%02X ch:%02X len:%d rssi:%d\n",
                         ts, pkt.source, pkt.dest,
                         pkt.packetId, pkt.flags,
                         pkt.channelHash, pkt.payloadLen,
                         pkt.rssi);
        }
        delay(10);
        _term.refresh();
    }

    // Consume the key that exits monitor mode
    if (_kb.available()) _kb.read();
    while (Serial.available()) Serial.read();

    _term.printlnColored("\nMonitor exited.", COLOR_INFO);
}

void Shell::_cmdWhoami(const char* args) {
    _term.println("root");
}

void Shell::_cmdHostname(const char* args) {
    _term.println(HOSTNAME);
}

void Shell::_cmdPwd(const char* args) {
    _term.println("/root");
}
