
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
    screen.fillScreen(TFT_BLACK);
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

bool initSD() 
{
    Serial.println("Initializing SD...");
    SD_SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SD_SPI, SD_FREQ)) 
    {
        Serial.println("SD Card Mount Failed");
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
constexpr int ITEM_HEIGHT = 32;
constexpr int MAX_ITEMS = 240 / ITEM_HEIGHT;
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
    screen.setTextSize(2);
    if (prev_selected != selected) screen.fillScreen(TFT_BLACK);
    const int size = files.size();
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;
        const char* filename = files[item].c_str();
        int y = i * ITEM_HEIGHT;
        if (item == selected)
        {
            screen.setTextColor(TFT_GREEN, TFT_BLACK);
            screen.drawString("> ", 10, y, 2);
            screen.drawString(filename, 30, y, 2);
        }
        else
        {
            screen.setTextColor(TFT_WHITE, TFT_BLACK); 
            screen.drawString(filename, 10, y, 2);
        }
    }

    prev_selected = selected;
}

void selectGame()
{
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

    // Target frame time: 16639µs (60.98 FPS)
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