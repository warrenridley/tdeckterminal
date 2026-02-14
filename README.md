# MeshOS — Terminal Firmware for LILYGO T-Deck Plus

A Linux-like terminal operating environment for the LILYGO T-Deck Plus that runs
directly on the ESP32-S3. Provides a shell with built-in **Meshtastic CLI** and
**MeshCore CLI** over the onboard SX1262 LoRa radio.

## What Is This?

The ESP32-S3 can't run a real Linux kernel (no MMU), so MeshOS is a
purpose-built firmware that *looks and feels* like a Linux terminal:

- Green-on-black terminal with blinking cursor on the 320×240 TFT
- Full BlackBerry-style keyboard input
- Trackball navigation / history scrolling
- USB serial passthrough (use it from a PC terminal too)
- Linux-style commands: `uname -a`, `free`, `ps`, `uptime`, `neofetch`, etc.
- **Meshtastic protocol** — send/receive messages, view nodes, traceroute
- **MeshCore protocol** — send/receive, ping, repeater mode, route discovery

## Hardware

| Component | Chip | Notes |
|-----------|------|-------|
| MCU | ESP32-S3 | 240 MHz dual-core, 16 MB flash, 8 MB PSRAM |
| Display | ST7789V | 320×240 IPS, SPI, landscape mode |
| Keyboard | BB Q10 style | I2C at 0x55 |
| LoRa Radio | SX1262 | 868/915 MHz, shared SPI bus |
| Trackball | 5-way | GPIO with debounce |
| Battery | LiPo | ADC on GPIO 4 |

## Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- USB-C cable
- LILYGO T-Deck Plus hardware

## Build & Flash

```bash
# Install PlatformIO CLI (if not already installed)
pip install platformio

# Clone / cd into this directory
cd testapp

# Build firmware
pio run

# Flash to device (connect via USB-C)
pio run -t upload

# Open serial monitor (also acts as a second terminal)
pio device monitor
```

## Shell Commands

### System

| Command | Description |
|---------|-------------|
| `help` | Show all available commands |
| `clear` | Clear terminal screen |
| `reboot` | Reboot the device |
| `uptime` | Show time since boot |
| `free` | Memory usage (Heap / PSRAM / Flash) |
| `uname -a` | Full system identification |
| `neofetch` | System info with ASCII art |
| `battery` | Battery voltage and percentage |
| `ifconfig` | Show radio interface details |
| `ps` | List running tasks |
| `dmesg` | Replay boot log |
| `history` | Command history |
| `scan` | 10-second RF scan |
| `monitor` | Live packet monitor (any key to exit) |
| `whoami` | Current user (root) |
| `hostname` | Device hostname |
| `pwd` | Working directory |
| `echo <text>` | Print text |
| `ls` | List SD card files |
| `cat <file>` | Read file from SD |

### Meshtastic (`mesh` prefix)

| Command | Description |
|---------|-------------|
| `mesh send <msg>` | Broadcast a text message |
| `mesh dm <id> <msg>` | Direct message to a node (hex ID) |
| `mesh nodes` | List all discovered nodes |
| `mesh info` | Radio & protocol status |
| `mesh channel [name]` | Get/set channel name |
| `mesh freq [mhz]` | Get/set frequency |
| `mesh power [dbm]` | Get/set TX power (-9 to 22) |
| `mesh trace <id>` | Traceroute to node |

### MeshCore (`core` prefix)

| Command | Description |
|---------|-------------|
| `core send <msg>` | Broadcast a message |
| `core dm <id> <msg>` | Direct message to a node |
| `core nodes` | List known MeshCore nodes |
| `core info` | Protocol info |
| `core ping <id>` | Ping a node (auto PONG reply) |
| `core route <id>` | Request route to node |
| `core repeater [on\|off]` | Toggle mesh repeater mode |
| `core config` | Show MeshCore configuration |

## Architecture

```
src/
├── main.cpp            # Setup, boot sequence, main loop
├── config.h            # All pin definitions and constants
├── terminal.h/cpp      # TFT terminal engine (53×30 chars)
├── keyboard.h/cpp      # I2C keyboard + trackball input
├── radio.h/cpp         # SX1262 LoRa radio abstraction
├── meshtastic_cli.h/cpp # Meshtastic protocol + CLI commands
├── meshcore_cli.h/cpp  # MeshCore protocol + CLI commands
└── shell.h/cpp         # Command shell with Linux-like builtins
```

## Radio Configuration

Default settings match **Meshtastic LongFast (NA)**:

| Parameter | Value |
|-----------|-------|
| Frequency | 906.875 MHz |
| Bandwidth | 250 kHz |
| Spreading Factor | SF11 |
| Coding Rate | 4/5 |
| TX Power | 22 dBm |
| Sync Word | 0x2B |

Change at runtime with `mesh freq` and `mesh power`, or edit defaults
in `config.h`.

## Extending

**Add SD card support:** Mount FAT32 in `setup()`, then wire `ls`/`cat`
commands to the filesystem.

**Full Meshtastic encryption:** Add protobuf encoding and AES-CTR
encryption to `meshtastic_cli.cpp` using the channel key.

**GPS integration:** If your T-Deck Plus has a GPS module, read NMEA and
add a `gps` command to the shell.

**Bluetooth:** The ESP32-S3 has BLE — add a `bt` command set for BLE
mesh or serial bridging.

## License

MIT — do whatever you want with it.
