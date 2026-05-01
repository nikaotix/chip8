#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>

#define RAM_SIZE 4096
#define NUMBER_REGISTERS 16
#define STACK_SIZE 16

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8 // dividing by 8 for 1-bit display stored in bytes.

#define FONT_SIZE 16 * 5 //0-F, each is 1byte wide, 5 tall.

// TODO: set up platform checks here!
#define HOST_LITTLE_ENDIAN // we're on intel so need to swap

typedef struct
{
    uint8_t ram[RAM_SIZE];
    uint8_t registers[NUMBER_REGISTERS];
    uint16_t iRegister;
    uint16_t programCounter;
    uint16_t stack[STACK_SIZE];
    uint16_t keys;
    uint8_t display[DISPLAY_SIZE];
    uint8_t stackPointer;
    uint8_t delayTimer;
    uint8_t soundTimer;
    bool displayChanged;
    bool waitingForKey;
    uint8_t keyRegister;
} chip8State;

inline uint16_t swapEndian(uint16_t i);
inline size_t screen_coords(size_t x, size_t y);
void step(chip8State *chip8);
void initialize(chip8State *chip8);

#endif