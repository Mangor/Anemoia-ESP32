<h1 align="center">
  <br>
  <img src="https://raw.githubusercontent.com/Shim06/Anemoia/main/assets/Anemoia.png" alt="Anemoia" width="150">
  <br>
  <b>Anemoia-ESP32</b>
  <br>
</h1>

<p align="center">
  Anemoia-ESP32 is a rewrite and port of the Anemoia Nintendo Entertainment System (NES) emulator running directly on the ESP32.  
  It is written in C++ using and designed to bring classic NES games to embedded hardware.  
  This project focuses on performance, being able to run the emulator at almost native speeds and with full audio emulation implemented. However, games with complex mappers may induce a small speed loss.
  <br/>
  Anemoia-ESP32 is available on GitHub under the <a href="https://github.com/Shim06/Anemoia-ESP32/blob/main/LICENSE" target="_blank">GNU General Public License v3.0 (GPLv3)</a>.
</p>

---

## Compatibility

As of now, Anemoia-ESP32 has implemented four major memory mappers, totalling to around 73% of the entire NES game catalogue. 
Feel free to open an issue if a game has glitches or fails to boot.

---

## Hardware

- **ESP32** 38-pin development board.  
- **MicroSD card module** for loading ROMs.  
- **320x240 TFT ST7789 LCD**. 
- **8 Tactile push buttons**

### Default Pin Setup
| Component         | Signal   | ESP32 Pin |
|-------------------|----------|-----------|
| **MicroSD Module**| MOSI     | 13        |
|                   | MISO     | 14        |
|                   | SCLK     | 26        |
|                   | CS       | 19        |
| **TFT Display**   | MOSI     | 23        |
|                   | MISO     | -1 (N/A)  |
|                   | SCLK     | 18        |
|                   | CS       | 4         |
|                   | DC       | 2         |
|                   | RST      | 5         |
| **Buttons**       | A        | 22        |
|                   | B        | 21        |
|                   | Left     | 0         |
|                   | Right    | 17        |
|                   | Up       | 16        |
|                   | Down     | 33        |
|                   | Start    | 32        |
|                   | Select   | 27        |

---

## Getting Started

1. Upload the `Anemoia-ESP32.ino` program into the ESP32
2. Put .nes game roms inside the root of a microSD card
3. Insert the microSD card into the microSD card module
4. Power on the ESP32 and select a game from the file select menu

## How to build and upload

### Step 1

Either use `git clone https://github.com/Shim06/Anemoia-ESP32.git` on the command line to clone the repository or use Code → Download zip button and extract to get the files.

### Step 2
1. Download and install the Arduino IDE. 
2. In <b> File → Preferences → Additional boards manager URLs </b> , add:
```cmd
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
3. Download the ESP32 board support through <b> Tools → Board → Boards Manager </b>. 
4. Download the `SdFat` and `TFT_eSPI` libraries from <b> Tools → Manage Libraries </b>.

### Step 3 - Configure TFT_eSPI
The emulator uses a custom display configuration for the ST7789 display.
1. Navigate to your Arduino Libraries folder:
(Default location): `Documents/Arduino/libraries/TFT_eSPI`
2. Open User_Setup_Select.h in a text editor.
3. Comment out any existing `#include` lines and add the following instead:
```C++
#include <User_Setups/Anemoia-ST7789.h>
```
4. Copy the provided `Anemoia-ST7789.h` file from this repository into
`TFT_eSPI/User_Setups/`. Optionally, edit the `#define` pins as desired.

### Step 4 - Apply custom build flags
1. Locate your ESP32 Arduino platform directory. This is typically at:
```cmd
\Users\{username}\AppData\Local\Arduino15\packages\esp32\hardware\esp32\{version}\
```
2. Copy the platform.local.txt file from this repository to that folder.
This file defines additional compiler flags and optimizations used by Anemoia-ESP32.

### Step 5 - Upload
1. Connect your ESP32 via USB.
2. In the Arduino IDE, go to <b> Tools → Board </b> and select your ESP32 board (e.g., ESP32 Dev Module).
3. Click Upload or press `Ctrl+U` to build and flash the emulator. Optionally, edit the `#define` pins as desired.

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3) - see the [LICENSE](LICENSE) file for more details.
