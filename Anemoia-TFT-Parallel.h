#define TFT_PARALLEL_8_BIT
#define ILI9341_DRIVER // Uncomment if using ILI9341 display
//#define ST7789_DRIVER // Uncomment if using ST7789 display

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF
#define TFT_DMA_ENABLED
#define SUPPORT_TRANSACTIONS
#define USE_DMA

#define TFT_DC	2
#define TFT_WR	4  
#define TFT_RD	-1  // Tie to high / 3V3
#define TFT_CS	-1	// Tie to low
#define TFT_RST	-1	// Connect to ESP32 EN pin

#define TFT_D0	14	// DB_8
#define TFT_D1	16	// DB_9  
#define TFT_D2	17	// DB_10
#define TFT_D3	18	// DB_11
#define TFT_D4	19	// DB_12
#define TFT_D5	23	// DB_13
#define TFT_D6	25	// DB_14
#define TFT_D7	33	// DB_15

#define LOAD_GLCD	// Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2	// Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
//#define LOAD_FONT4	// Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
//#define LOAD_FONT6	// Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
//#define LOAD_FONT7	// Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
//#define LOAD_FONT8	// Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_GFXFF	// FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
//#define SMOOTH_FONT // Comment out to stop the SPIFFS filing system and smooth font code being loaded (will save ~20kbytes of FLASH)