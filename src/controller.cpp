#include "../config.h"
#include "controller.h"
#include "core/bus.h"
#include <Arduino.h>

uint8_t controllerRead()
{
    uint8_t state = 0;

#if defined(CONTROLLER_GPIO)
    if (digitalRead(A_BUTTON)      == LOW) state |= CONTROLLER::A;
    if (digitalRead(B_BUTTON)      == LOW) state |= CONTROLLER::B;
    if (digitalRead(SELECT_BUTTON) == LOW) state |= CONTROLLER::Select;
    if (digitalRead(START_BUTTON)  == LOW) state |= CONTROLLER::Start;
    if (digitalRead(UP_BUTTON)     == LOW) state |= CONTROLLER::Up;
    if (digitalRead(DOWN_BUTTON)   == LOW) state |= CONTROLLER::Down;
    if (digitalRead(LEFT_BUTTON)   == LOW) state |= CONTROLLER::Left;
    if (digitalRead(RIGHT_BUTTON)  == LOW) state |= CONTROLLER::Right;

#elif defined(CONTROLLER_NES)
    digitalWrite(CONTROLLER_NES_LATCH, HIGH);
    delayMicroseconds(12);
    digitalWrite(CONTROLLER_NES_LATCH, LOW);
    delayMicroseconds(6);

    for (int i = 0; i < 8; i++)
    {
        if (digitalRead(CONTROLLER_NES_DATA) == LOW) state |= (1 << i);
        digitalWrite(CONTROLLER_NES_CLK, LOW);
        delayMicroseconds(6);
        digitalWrite(CONTROLLER_NES_CLK, HIGH);
        delayMicroseconds(6);
    }

#elif defined(CONTROLLER_SNES)
    // SNES bits
    // 0 - B
    // 1 - Y
    // 2 - Select
    // 3 - Start
    // 4 - Up
    // 5 - Down
    // 6 - Left
    // 7 - Right
    // 8 - A
    // 9 - X
    // 10 - L
    // 11 - R

    uint16_t snes_state = 0;
    digitalWrite(CONTROLLER_SNES_LATCH, HIGH);
    delayMicroseconds(12);
    digitalWrite(CONTROLLER_SNES_LATCH, LOW);
    delayMicroseconds(6);

    for (int i = 0; i < 12; i++)
    {
        if (digitalRead(CONTROLLER_SNES_DATA) == LOW) snes_state |= (1 << i);
        digitalWrite(CONTROLLER_SNES_CLK, LOW);
        delayMicroseconds(6);
        digitalWrite(CONTROLLER_SNES_CLK, HIGH);
        delayMicroseconds(6);
    }

    // NES compatible bits
    state |= snes_state & 0xFF;

    // Map extra bits to A and B buttons
    if (snes_state & (1 << 8)) state |= CONTROLLER::A;
    if (snes_state & (1 << 9)) state |= CONTROLLER::B;
    if (snes_state & (1 << 10)) state |= CONTROLLER::B;
    if (snes_state & (1 << 11)) state |= CONTROLLER::A;

#else
    #error "No controller type selected"
#endif

    return state;
}

bool isDownPressed(CONTROLLER button)
{
    return (controllerRead() & button) != 0;
}

void initController()
{
#if defined(CONTROLLER_GPIO)
    pinMode(A_BUTTON, INPUT_PULLUP);
    pinMode(B_BUTTON, INPUT_PULLUP);
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    pinMode(UP_BUTTON, INPUT_PULLUP);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_BUTTON, INPUT_PULLUP);

#elif defined(CONTROLLER_NES)
    pinMode(CONTROLLER_NES_CLK, OUTPUT);
    pinMode(CONTROLLER_NES_LATCH, OUTPUT);
    pinMode(CONTROLLER_NES_DATA, INPUT);

#elif defined(CONTROLLER_SNES)
    pinMode(CONTROLLER_SNES_CLK, OUTPUT);
    pinMode(CONTROLLER_SNES_LATCH, OUTPUT);
    pinMode(CONTROLLER_SNES_DATA, INPUT);

#else
    #error "No controller type selected"
#endif
}
