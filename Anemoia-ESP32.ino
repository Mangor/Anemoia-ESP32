
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <vector>

#include "bus.h"
#include "config.h"
#include "driver/i2s.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#define BG_COLOR 0x0015
#define BAR_COLOR 0xAD55
#define TEXT_COLOR 0xFFFF
#define TEXT2_COLOR 0xA800
#define SELECTED_TEXT_COLOR 0x57CA

TFT_eSPI screen = TFT_eSPI();
SPIClass SD_SPI(HSPI);
std::vector<std::string> files;
Cartridge* cart;
void setup() 
{
    // Turn off Wifi and Bluetooth to reduce CPU overhead
    #ifdef DEBUG
        Serial.begin(115200);
    #endif
    
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    esp_wifi_deinit();
    btStop();
    esp_bt_controller_disable();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    setupI2SDAC();

    // Initialize TFT screen
    screen.begin();
    screen.setRotation(SCREEN_ROTATION);
    screen.initDMA();
    screen.fillScreen(BG_COLOR);
    screen.startWrite();
    screen.setSwapBytes(SCREEN_SWAP_BYTES);

    // Initialize microsd card
    if(!initSD()) while (true);

    // Setup buttons
    pinMode(A_BUTTON, INPUT_PULLUP);
    pinMode(B_BUTTON, INPUT_PULLUP);
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    pinMode(UP_BUTTON, INPUT_PULLUP);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_BUTTON, INPUT_PULLUP);

    selectGame();

}

void loop() 
{
    emulate();
}

#ifdef DEBUG
    unsigned long last_frame_time = 0;
    unsigned long current_frame_time = 0;
    unsigned long total_frame_time = 0;
    unsigned long frame_count = 0;
#endif
IRAM_ATTR void emulate()
{
    Bus nes;
    nes.insertCartridge(cart);
    nes.connectScreen(&screen);
    nes.reset();

    xTaskCreatePinnedToCore(
    apuTask,
    "APU Task",
    1024,
    &nes.cpu.apu,
    1,
    NULL,
    0
    );

    #ifdef DEBUG
        last_frame_time = esp_timer_get_time();
    #endif

    // Target frame time: 16639µs (60.098 FPS)
    #define FRAME_TIME 16639
    uint64_t next_frame = esp_timer_get_time();
    // Emulation Loop
    while (true) 
    {
        // Read button input
        nes.controller = 0;
        if (digitalRead(A_BUTTON)      == LOW) nes.controller |= (Bus::CONTROLLER::A);
        if (digitalRead(B_BUTTON)      == LOW) nes.controller |= (Bus::CONTROLLER::B);
        if (digitalRead(SELECT_BUTTON) == LOW) nes.controller |= (Bus::CONTROLLER::Select);
        if (digitalRead(START_BUTTON)  == LOW) nes.controller |= (Bus::CONTROLLER::Start);  
        if (digitalRead(UP_BUTTON)     == LOW) nes.controller |= (Bus::CONTROLLER::Up);
        if (digitalRead(DOWN_BUTTON)   == LOW) nes.controller |= (Bus::CONTROLLER::Down);
        if (digitalRead(LEFT_BUTTON)   == LOW) nes.controller |= (Bus::CONTROLLER::Left);
        if (digitalRead(RIGHT_BUTTON)  == LOW) nes.controller |= (Bus::CONTROLLER::Right);

        // Generate one frame
        nes.clock();

        #ifdef DEBUG
            current_frame_time = esp_timer_get_time();
            total_frame_time += (current_frame_time - last_frame_time);
            frame_count++;

            if ((frame_count & 63) == 0)
            {
                float avg_fps = (1000000.0 * frame_count) / total_frame_time;
                Serial.printf("FPS: %.2f\n", avg_fps);
                total_frame_time = 0;
                frame_count = 0;
            }

            last_frame_time = current_frame_time;
        #endif

        // Frame limiting
        uint64_t now = esp_timer_get_time();
        if (now < next_frame) ets_delay_us(next_frame - now);
        next_frame += FRAME_TIME;
    }
    #undef FRAME_TIME
}

bool initSD() 
{
    Serial.println("Initializing SD...");
    SD_SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SD_SPI, SD_FREQ)) 
    {
        #ifdef DEBUG
            Serial.println("SD Card Mount Failed");
        #endif

        screen.setTextSize(2);
        const char* txt1 = "SD Init failed!";
        const char* txt2 = "Insert SD card or";
        const char* txt3 = "lower SD frequency";
        const char* txt4 = "in config.h";
        int w1 = screen.textWidth(txt1, 2);
        int w2 = screen.textWidth(txt2, 2);
        int w3 = screen.textWidth(txt3, 2);
        int w4 = screen.textWidth(txt4, 2);
        
        int x1 = (320 - w1) / 2;
        int x2 = (320 - w2) / 2;
        int x3 = (320 - w3) / 2;
        int x4 = (320 - w4) / 2;

        screen.drawString(txt1, x1, 56, 2);
        screen.drawString(txt2, x2, 88, 2);
        screen.drawString(txt3, x3, 120, 2);
        screen.drawString(txt4, x4, 152, 2);
        return false;
    }

    return true;
}

void setupI2SDAC()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
}

void apuTask(void* param) 
{
    Apu2A03* apu = (Apu2A03*)param;

    while (true)
    {
        apu->clock();
    }
}

int selected = 0;
int prev_selected = 0;
int scroll_offset = 0;
constexpr int ITEM_HEIGHT = 12;
int MAX_ITEMS = (screen.width() - 56) / ITEM_HEIGHT;
void getNesFiles()
{
    File root = SD.open("/");
    while (true)
    {
        File file = root.openNextFile();
        if (!file) break;
        if (!file.isDirectory())
        {
            std::string filename = file.name();
            if (filename.ends_with(".nes")) files.push_back(filename);
        }

        file.close();
    }

    root.close();
}

void drawFileList()
{
    screen.setTextSize(1);
    if (prev_selected != selected) screen.fillRect(10, 32, screen.width() - 20, screen.height() - 64, BG_COLOR);
    const int size = files.size();
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;
        const char* filename = files[item].c_str();
        int y = i * ITEM_HEIGHT + 32;
        if (item == selected)
        {
            screen.setTextColor(SELECTED_TEXT_COLOR, BG_COLOR);
            screen.drawString(filename, 14, y, 1);
        }
        else
        {
            screen.setTextColor(TEXT_COLOR, BG_COLOR); 
            screen.drawString(filename, 14, y, 1);
        }
    }

    prev_selected = selected;
}

void selectGame()
{
    drawBars();
    drawWindowBox(2, 20, screen.width() - 4, screen.height() - 40);
    getNesFiles();
    drawFileList();
    unsigned int last_input_time = 0;
    constexpr unsigned int delay = 250; 
    const int size = files.size();

    while (true)
    {
        unsigned int now = millis();

        if (now - last_input_time > delay)
        {
            if (digitalRead(UP_BUTTON) == LOW) 
            {
                selected--;
                if (selected < 0)
                {
                    selected = size - 1;
                    scroll_offset = selected - MAX_ITEMS + 1;
                }
                else if (selected < scroll_offset)  scroll_offset = selected; 
                drawFileList();
                last_input_time = now;
            }

            if (digitalRead(DOWN_BUTTON) == LOW) 
            {
                selected++; 
                if (selected > size - 1)
                {
                    selected = 0;
                    scroll_offset = selected;
                }
                else if (selected >= scroll_offset + MAX_ITEMS) scroll_offset = selected - MAX_ITEMS + 1;
                drawFileList();
                last_input_time = now;
            }
            
        }

        if (digitalRead(A_BUTTON) == LOW && (selected >= 0 && selected < size))
        {
            cart = new Cartridge(("/" + files[selected]).c_str());
            return;
        }
    }
}

void drawWindowBox(int x, int y, int w, int h) 
{
    screen.drawRect(x, y, w, h, TFT_WHITE);
    screen.drawRect(x+1, y, w-2, h, TFT_WHITE);

    screen.drawRect(x+4, y+3, w-8, h-7, TFT_WHITE);
    screen.drawRect(x+5, y+3, w-10, h-7, TFT_WHITE);

    char* text1 = " ANEMOIA.CPP ";
    screen.setTextColor(TEXT_COLOR, BG_COLOR);
    screen.setCursor((screen.width() - screen.textWidth(text1)) / 2, 20);
    screen.print(text1);
}

void drawBars() 
{
    // Top bar
    screen.fillRect(0, 0, screen.width(), 16, BAR_COLOR);
    screen.setTextColor(TFT_BLACK, BAR_COLOR);

    char* text1 = "ANEMOIA-ESP32";
    screen.setCursor((screen.width() - screen.textWidth(text1)) / 2, 4);
    screen.print(text1);

    // Bottom bar
    screen.fillRect(0, screen.height() - 16, screen.width(), 16, BAR_COLOR);
    screen.setTextColor(TFT_BLACK, BAR_COLOR);

    int y = screen.height() - 12;
    int x = 4;

    screen.setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen.setCursor(x, y);
    screen.print("Up/Down");

    screen.setTextColor(TFT_BLACK, BAR_COLOR);
    screen.print(" Select   ");

    screen.setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen.print("A");

    screen.setTextColor(TFT_BLACK, BAR_COLOR);
    screen.print(" Start");
}