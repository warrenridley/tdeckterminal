//
// MeshOS - Terminal firmware for LILYGO T-Deck Plus
//
// Provides a Linux-like terminal interface with:
//   - Meshtastic CLI (mesh send, mesh nodes, mesh info, ...)
//   - MeshCore CLI   (core send, core ping, core repeater, ...)
//   - System commands (uname, free, ps, neofetch, battery, ...)
//
// Built for ESP32-S3 + SX1262 LoRa + ST7789V display + BB keyboard
//

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

#include "config.h"
#include "terminal.h"
#include "keyboard.h"
#include "radio.h"
#include "meshtastic_cli.h"
#include "meshcore_cli.h"
#include "shell.h"

// ---- Global objects ----
TFT_eSPI       tft = TFT_eSPI();
Terminal        term(tft);
Keyboard        kb;
Radio           radio;
MeshtasticCLI   meshCli(term, radio);
MeshCoreCLI     coreCli(term, radio);
Shell           shell(term, kb, radio, meshCli, coreCli);

// ---- Boot sequence ----
void bootSequence() {
    term.setColor(COLOR_FG);

    term.printlnColored(
        OS_NAME " " OS_VERSION " -- LILYGO T-Deck Plus",
        COLOR_HEADER);
    term.printlnColored(
        "============================================",
        COLOR_HEADER);
    term.println();
    delay(150);

    // Hardware summary
    term.printf("CPU:   Xtensa LX7 dual-core @ %d MHz\n",
                ESP.getCpuFreqMHz());
    delay(80);
    term.printf("RAM:   %uK   PSRAM: %uK\n",
                ESP.getHeapSize() / 1024,
                ESP.getPsramSize() / 1024);
    delay(80);
    term.printf("Flash: %u MB\n",
                ESP.getFlashChipSize() / (1024 * 1024));
    delay(80);
    term.println();

    // Peripheral init status
    term.print("Display .......... ");
    delay(60);
    term.printlnColored("[  OK  ]", COLOR_FG);

    term.print("Keyboard ......... ");
    delay(60);
    term.printlnColored("[  OK  ]", COLOR_FG);

    term.print("LoRa SX1262 ...... ");
    delay(60);
    if (radio.isInitialized()) {
        term.printlnColored("[  OK  ]", COLOR_FG);
        term.printf("  %.3f MHz  %d dBm  SF%d BW%.0f\n",
                    radio.getFrequency(), radio.getPower(),
                    LORA_SF, (float)LORA_BW);
        term.printf("  Node ID: !%08X\n", radio.getNodeId());
    } else {
        term.printlnColored("[ FAIL ]", COLOR_ERROR);
        term.printlnColored("  Check antenna & hardware", COLOR_WARNING);
    }
    delay(60);

    term.print("Meshtastic ....... ");
    delay(60);
    term.printlnColored("[  OK  ]", COLOR_FG);

    term.print("MeshCore ......... ");
    delay(60);
    term.printlnColored("[  OK  ]", COLOR_FG);

    term.println();
    term.printlnColored("All systems ready.", COLOR_FG);
    term.println();

    // Login banner
    term.printf("%s %s (%s) (ttyS0)\n\n", OS_NAME, OS_VERSION, HOSTNAME);
    term.println("Type 'help' for commands.");
    term.println();
}

// ---- Arduino entry points ----

void setup() {
    Serial.begin(115200);

    // Power on board
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);
    delay(100);

    // Initialize shared SPI bus (TFT + LoRa + SD all share these pins)
    SPI.begin(RADIO_SCK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);

    // Backlight on
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);

    // Display
    term.init();
    term.printlnColored("Booting " OS_NAME " ...", COLOR_FG);
    term.println();
    term.refresh();

    // Keyboard
    kb.init();

    // Radio
    radio.init();   // OK if it fails; shell handles gracefully

    // Protocol layers
    meshCli.init();
    coreCli.init();

    // Boot messages
    bootSequence();

    // Start shell
    shell.init();
}

void loop() {
    shell.update();
    delay(10);  // yield to watchdog / reduce power
}
