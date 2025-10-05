#ifndef CONFIG_H
#define CONFIG_H

// Screen Configuration
#define SCREEN_ROTATION 1 // Screen orientation: 1 or 3 (1 = landscape, 3 = landscape flipped)
#define SCREEN_SWAP_BYTES true // Set to false if colors appear wrong

// MicroSD card module Pins
#define SD_MOSI_PIN 13
#define SD_MISO_PIN 14
#define SD_SCLK_PIN 26
#define SD_CS_PIN 19

// Button pins
#define A_BUTTON 22
#define B_BUTTON 21
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 17
#define UP_BUTTON 16
#define DOWN_BUTTON 33
#define START_BUTTON 32
#define SELECT_BUTTON 27

// Sound sample rate
#define SAMPLE_RATE 44100

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial

#endif