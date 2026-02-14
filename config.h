#ifndef CONFIG_H
#define CONFIG_H

// ========================================================
// MeshOS Configuration for LILYGO T-Deck Plus
// ========================================================

// ====== Board Power ======
#define BOARD_POWERON           10

// ====== Display (ST7789V 320x240) ======
#define TFT_BL_PIN              42
#define TFT_CS_PIN              12
#define TFT_DC_PIN              11

// ====== Keyboard (I2C) ======
#define BOARD_I2C_SDA           18
#define BOARD_I2C_SCL           8
#define KB_I2C_ADDR             0x55
#define KB_INT_PIN              46

// ====== LoRa Radio (SX1262) ======
#define RADIO_CS_PIN            9
#define RADIO_RST_PIN           17
#define RADIO_DIO1_PIN          45
#define RADIO_BUSY_PIN          13
#define RADIO_MOSI_PIN          41
#define RADIO_MISO_PIN          38
#define RADIO_SCK_PIN           40

// ====== SD Card ======
#define SD_CS_PIN               39

// ====== Trackball ======
#define TRACKBALL_UP_PIN        3
#define TRACKBALL_DOWN_PIN      15
#define TRACKBALL_LEFT_PIN      1
#define TRACKBALL_RIGHT_PIN     2
#define TRACKBALL_CLICK_PIN     0

// ====== Battery ======
#define BAT_ADC_PIN             4

// ====== Audio (ES7210 + I2S) ======
#define I2S_WS_PIN              5
#define I2S_BCK_PIN             7
#define I2S_DOUT_PIN            6

// ====== Terminal Configuration ======
#define TERM_FONT_W             6       // GLCD font character width
#define TERM_FONT_H             8       // GLCD font character height
#define SCREEN_WIDTH            320
#define SCREEN_HEIGHT           240
#define TERM_COLS               (SCREEN_WIDTH / TERM_FONT_W)    // 53
#define TERM_ROWS               (SCREEN_HEIGHT / TERM_FONT_H)   // 30

// ====== Terminal Color Scheme (RGB565) ======
#define COLOR_BG                0x0000  // Black
#define COLOR_FG                0x07E0  // Green (classic terminal)
#define COLOR_PROMPT            0x07FF  // Cyan
#define COLOR_ERROR             0xF800  // Red
#define COLOR_WARNING           0xFFE0  // Yellow
#define COLOR_INFO              0x001F  // Blue
#define COLOR_HEADER            0xF81F  // Magenta
#define COLOR_DIM               0x3186  // Dark gray
#define COLOR_WHITE             0xFFFF  // White

// ====== LoRa Radio Defaults (Meshtastic LongFast NA) ======
#define LORA_FREQ               906.875 // MHz
#define LORA_BW                 250.0   // kHz
#define LORA_SF                 11      // Spreading factor
#define LORA_CR                 5       // Coding rate (4/5)
#define LORA_SYNC               0x2B    // Meshtastic sync word
#define LORA_POWER              22      // dBm
#define LORA_PREAMBLE           16      // Preamble symbols

// ====== Meshtastic Protocol ======
#define MESH_BROADCAST          0xFFFFFFFF
#define MESH_MAX_PAYLOAD        237
#define MESH_HEADER_LEN         16

// ====== System Identity ======
#define HOSTNAME                "tdeck"
#define OS_NAME                 "MeshOS"
#define OS_VERSION              "1.0.0"
#define KERNEL_VERSION          "6.1.0-mesh"

// ====== Limits ======
#define MAX_NODES               50
#define CMD_HISTORY_SIZE        20
#define CMD_MAX_LEN             128
#define PRINTF_BUF_SIZE         256

#endif // CONFIG_H
