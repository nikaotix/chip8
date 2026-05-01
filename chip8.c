// Chip-8 simulator, currently only targeting regular Chip-8
// based on documentation at http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

// Chip-8 is big-endian (MSB first)
// writing this on x86_64 which is little-endian (LSB first)
// so all 16-bit reads/writes will need to be swapped.
// program counter, stack, and I register are 16-bit.
// whenever these are set from RAM, use swapEndian!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

#define COSMAC

uint8_t font[FONT_SIZE] =
    {
        0xF0,
        0x90,
        0x90,
        0x90,
        0xF0,
        0x20,
        0x60,
        0x20,
        0x20,
        0x70,
        0xF0,
        0x10,
        0xF0,
        0x80,
        0xF0,
        0xF0,
        0x10,
        0xF0,
        0x10,
        0xF0,
        0x90,
        0x90,
        0xF0,
        0x10,
        0x10,
        0xF0,
        0x80,
        0xF0,
        0x10,
        0xF0,
        0xF0,
        0x80,
        0xF0,
        0x90,
        0xF0,
        0xF0,
        0x10,
        0x20,
        0x40,
        0x40,
        0xF0,
        0x90,
        0xF0,
        0x90,
        0xF0,
        0xF0,
        0x90,
        0xF0,
        0x10,
        0xF0,
        0xF0,
        0x90,
        0xF0,
        0x90,
        0x90,
        0xE0,
        0x90,
        0xE0,
        0x90,
        0xE0,
        0xF0,
        0x80,
        0x80,
        0x80,
        0xF0,
        0xE0,
        0x90,
        0x90,
        0x90,
        0xE0,
        0xF0,
        0x80,
        0xF0,
        0x80,
        0xF0,
        0xF0,
        0x80,
        0xF0,
        0x80,
        0x80,
};

uint8_t leftMaskTable[8] =
    {
        0xFF,
        0x7F,
        0x3F,
        0x1F,
        0x0F,
        0x07,
        0x03,
        0x01};

uint8_t rightMaskTable[8] =
    {
        0x00,
        0x80,
        0xC0,
        0xE0,
        0xF0,
        0xF8,
        0xFC,
        0xFE};

inline uint16_t swapEndian(uint16_t i)
{
#ifdef HOST_LITTLE_ENDIAN
    uint16_t temp = (i & 0xFF) << 8;
    temp += (i >> 8) & 0xFF;
    return temp;
#endif
#ifndef HOST_LITTLE_ENDIAN
    return i;
#endif
}

inline size_t screen_coords(size_t x, size_t y)
{
    return (((x % DISPLAY_WIDTH) / 8) + ((y % DISPLAY_HEIGHT) * (DISPLAY_WIDTH / 8)));
}

void screen_coords_test()
{
    if (screen_coords(1, 0) != 0)
    {
        printf("Screen coords test failed lmao\n");
        exit(30);
    }
    if (screen_coords(0, 1) != 8)
    {
        printf("Screen coords test failed lmao\n");
        exit(30);
    }
    if (screen_coords(1, 1) != 8)
    {
        printf("Screen coords test failed lmao\n");
        exit(30);
    }
}
// init clears all machine state and loads the font into the machine memory.
void initialize(chip8State *chip8)
{
    memset(chip8, 0, sizeof(chip8State));
    memcpy_s(chip8->ram, RAM_SIZE, font, FONT_SIZE);
}
// step runs a single instruction on the chip8 state.
void step(chip8State *chip8)
{
    // clear displayChanged for the new instruction.
    chip8->displayChanged = false;

    // TODO: write function that combines high and low bytes for big/little endian machines
    uint16_t instruction = ((uint16_t)chip8->ram[chip8->programCounter] << 8) + chip8->ram[chip8->programCounter + 1];
    chip8->programCounter += 2;
    uint8_t nibble = instruction >> 12;
    nibble &= 0xF;
    switch (nibble)
    {
    case 0:
        if (instruction == 0x00E0) // CLS
        {
            chip8->displayChanged = true;
            memset(chip8->display, 0, DISPLAY_SIZE);
        }
        else if (instruction == 0x00EE) // RET
        {
            chip8->programCounter = chip8->stack[chip8->stackPointer];
            chip8->stackPointer--;
            chip8->stackPointer %= 16;
        }
        break;

    case 1: // JP
        chip8->programCounter = (instruction & 0x0FFF);
        break;

    case 2: // CALL
        chip8->stackPointer++;
        chip8->stackPointer %= 16;
        chip8->stack[chip8->stackPointer] = chip8->programCounter;
        chip8->programCounter = instruction & 0x0FFF;
        break;

    case 3: // SE register/byte
        if (chip8->registers[(instruction & 0x0F00) >> 8] == (instruction & 0xFF))
            chip8->programCounter += 2;
        break;

    case 4: // SNE register/byte
        if (chip8->registers[(instruction & 0x0F00) >> 8] != (instruction & 0xFF))
            chip8->programCounter += 2;
        break;

    case 5: // SE register/register
        if (chip8->registers[(instruction & 0x0F00) >> 8] == chip8->registers[(instruction & 0x00F0) >> 4])
            chip8->programCounter += 2;
        break;

    case 6: // LD Vx byte
        chip8->registers[(instruction & 0x0F00) >> 8] = (instruction & 0xFF);
        break;

    case 7: // ADD Vx byte
        chip8->registers[(instruction & 0x0F00) >> 8] += (instruction & 0xFF);
        break;

    case 8: // register math
        switch (instruction & 0xF)
        {
        case 0: // LD Vx Vy
            chip8->registers[(instruction & 0x0F00) >> 8] = chip8->registers[(instruction & 0x00F0) >> 4];
            break;

        case 1: // OR Vx Vy
            chip8->registers[(instruction & 0x0F00) >> 8] = chip8->registers[(instruction & 0x0F00) >> 8] | chip8->registers[(instruction & 0x00F0) >> 4];
#ifdef COSMAC
            chip8->registers[0xF] = 0;
#endif
            break;

        case 2: // AND Vx Vy
            chip8->registers[(instruction & 0x0F00) >> 8] = chip8->registers[(instruction & 0x0F00) >> 8] & chip8->registers[(instruction & 0x00F0) >> 4];
#ifdef COSMAC
            chip8->registers[0xF] = 0;
#endif
            break;

        case 3: // XOR Vx Vy
            chip8->registers[(instruction & 0x0F00) >> 8] = chip8->registers[(instruction & 0x0F00) >> 8] ^ chip8->registers[(instruction & 0x00F0) >> 4];
#ifdef COSMAC
            chip8->registers[0xF] = 0;
#endif
            break;

        case 4: // ADD Vx Vy
        {
            uint16_t sum = chip8->registers[(instruction & 0x0F00) >> 8] + chip8->registers[(instruction & 0x00F0) >> 4];

            chip8->registers[(instruction & 0x0F00) >> 8] = sum & 0xFF;
            if (sum > 255)
            {
                chip8->registers[0xF] = 1;
            }
            else
            {
                chip8->registers[0xF] = 0;
            }
        }
        break;

        case 5: // SUB Vx Vy
        {
            uint16_t diff = chip8->registers[(instruction & 0x0F00) >> 8] - chip8->registers[(instruction & 0x00F0) >> 4];
            uint8_t carry;
            if (chip8->registers[(instruction & 0x0F00) >> 8] >= chip8->registers[(instruction & 0x00F0) >> 4])
            {
                carry = 1;
            }
            else
            {
                carry = 0;
            }
            chip8->registers[(instruction & 0x0F00) >> 8] = diff & 0xFF;
            chip8->registers[0xF] = carry;
        }
        break;

        case 6: // SHR Vx Vy
        {
#ifdef COSMAC
            chip8->registers[(instruction & 0x0F00) >> 8] = chip8->registers[(instruction & 0x00F0) >> 4];
#endif
            uint8_t carry = chip8->registers[(instruction & 0x0F00)] & 0x1;
            chip8->registers[(instruction & 0x0F00) >> 8] = (chip8->registers[(instruction & 0x0F00) >> 8] >> 1) & 0x7F;
            chip8->registers[0xF] = carry;
        }
        break;

        case 7: // SUBN Vx Vy (Vx is set to Vy-Vx)
        {
            uint16_t diff = chip8->registers[(instruction & 0x00F0) >> 4] - chip8->registers[(instruction & 0x0F00) >> 8];
            uint8_t carry;
            if (chip8->registers[(instruction & 0x0F00) >> 8] <= chip8->registers[(instruction & 0x00F0) >> 4])
            {
                carry = 1;
            }
            else
            {
                carry = 0;
            }
            chip8->registers[(instruction & 0x0F00) >> 8] = diff & 0xFF;
            chip8->registers[0xF] = carry;
        }
        break;

        case 0xE: // SHL Vx Vy
        {
#ifdef COSMAC
            chip8->registers[(instruction & 0x0F00) >> 8] = chip8->registers[(instruction & 0x00F0) >> 4];
#endif
            uint8_t carry = ((chip8->registers[(instruction & 0x0F00)] & 0x80) >> 7) & 0x1;
            chip8->registers[(instruction & 0x0F00) >> 8] = (chip8->registers[(instruction & 0x0F00) >> 8] << 1) & 0xFE;
            chip8->registers[0xF] = carry;
        }
        break;

        default:
            break;
        }
        break;

    case 9: // SNE register
        if (chip8->registers[(instruction & 0x0F00) >> 8] != chip8->registers[(instruction & 0x00F0) >> 4])
            chip8->programCounter += 2;
        break;

    case 0xA: // LD I from address
        chip8->iRegister = instruction & 0xFFF;
        break;

    case 0xB: // JP address + V0
        chip8->programCounter = (instruction & 0xFFF) + chip8->registers[0];
        break;

    case 0xC: // RND Vx & byte
        chip8->registers[(instruction & 0xF00) >> 8] = rand() & (instruction & 0xFF);
        break;

    case 0xD: // DRW sprite
        chip8->displayChanged = true;
        uint8_t x, y, bytes;
        chip8->registers[0xF] = 0;
        x = chip8->registers[(instruction & 0xF00) >> 8] % DISPLAY_WIDTH;
        y = chip8->registers[(instruction & 0xF0) >> 4] % DISPLAY_HEIGHT;
        bytes = (instruction & 0xF);
        uint8_t bitOffset = x % 8;
        if (bitOffset != 0) // sprite is not aligned to byte boundaries
        {
            for (size_t i = 0; i < bytes; i++)
            {
                uint8_t left = (chip8->ram[chip8->iRegister + i] >> bitOffset) & leftMaskTable[bitOffset];
                uint8_t right = (chip8->ram[chip8->iRegister + i] << (8 - bitOffset)) & rightMaskTable[bitOffset];
                // draw left half of sprite
                if (chip8->display[screen_coords(x, y)] & left)
                {
                    chip8->registers[0xF] = 1;
                }
                chip8->display[screen_coords(x, y)] ^= left;
                // draw right half of sprite (if it exists)
                if ((x + 8) < DISPLAY_WIDTH)
                {
                    if (chip8->display[screen_coords(x + 8, y)] & right)
                    {
                        chip8->registers[0xF] = 1;
                    }
                    chip8->display[screen_coords(x + 8, y)] ^= right;
                }
                y++;
                if (y >= DISPLAY_HEIGHT)
                {
                    break;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < bytes; i++)
            {
                uint8_t byte = chip8->ram[chip8->iRegister + i];
                if (chip8->display[screen_coords(x, y)] & byte)
                {
                    chip8->registers[0xF] = 1;
                }
                chip8->display[screen_coords(x, y)] ^= byte;
                y++;
                if (y >= DISPLAY_HEIGHT)
                {
                    break;
                }
            }
        }

        break;

    case 0xE: // keypad checks
              // Note that we're getting a 0x0-0xF value from the register specified by the instruction,
              // but the chip8->keys value stores each key as a single bit.
        uint16_t keyCheck = (0x1 << ((chip8->registers[(instruction & 0xF00) >> 8]) & 0xF));
        switch (instruction & 0xFF)
        {
        case 0x9E: // skip if pressed
            if (keyCheck & chip8->keys)
            {
                chip8->programCounter += 2;
            }
            break;

        case 0xA1: // skip if not pressed
            if (keyCheck & ~chip8->keys)
            {
                chip8->programCounter += 2;
            }
            break;
        }
        break;
    case 0xF: // timers and display utilities
        switch (instruction & 0xFF)
        {
        case 0x07: // LD Vx, DT
            chip8->registers[(instruction & 0xF00) >> 8] = chip8->delayTimer;
            break;

        case 0x0A: // LD Vx, K - This signals to the outside framework to not step the chip8 until a key is pressed
                   // and to put the value of the key pressed into the register specified by keyRegister.
                   // I assume the 'value' is 0x0-F.
            chip8->waitingForKey = true;
            chip8->keyRegister = (instruction & 0xF00) >> 8;
            break;

        case 0x15: // LD DT, Vx
            chip8->delayTimer = chip8->registers[(instruction & 0xF00) >> 8];
            break;

        case 0x18: // LD ST, Vx
            chip8->soundTimer = chip8->registers[(instruction & 0xF00) >> 8];
            break;

        case 0x1E: // ADD I, Vx
            chip8->iRegister += chip8->registers[(instruction & 0xF00) >> 8];
            break;

        case 0x29: // LD F, Vx
                   // Font will be stored at 0x00 + value * 4.
            chip8->iRegister = chip8->registers[(instruction & 0xF00) >> 8] << 2;
            break;

        case 0x33: // LD B, Vx
                   // this stores Vx as a BCD value in memory. hundreds, tens, ones.
            uint8_t val = chip8->registers[(instruction & 0xF00) >> 8];
            chip8->ram[chip8->iRegister] = val / 100;
            chip8->ram[chip8->iRegister + 1] = (val % 100) / 10;
            chip8->ram[chip8->iRegister + 2] = (val) % 10;
            break;

        case 0x55: // LD [I], Vx
            // Note that CHIP-8/CHIP-48 increment the iRegister, but SCHIP-8 does not.
            // I'm implementing CHIP-8 with the SCHIP-8 style instructions.
            for (size_t i = 0; i <= ((instruction & 0xF00) >> 8); i++)
            {
                chip8->ram[chip8->iRegister + i] = chip8->registers[i];
            }
#ifdef COSMAC
            chip8->iRegister += ((instruction & 0xF00) >> 8) + 1;
#endif
            break;

        case 0x65: // LD Vx, [I]
            for (size_t i = 0; i <= ((instruction & 0xF00) >> 8); i++)
            {
                chip8->registers[i] = chip8->ram[chip8->iRegister + i];
            }
#ifdef COSMAC
            chip8->iRegister += ((instruction & 0xF00) >> 8) + 1;
#endif
            break;
        }
        break;
    }
}