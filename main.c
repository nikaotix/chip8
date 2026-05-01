#include <stdio.h>
#include <stdlib.h>

#include "SDL3\SDL.h"
#include "SDL3\SDL_video.h"
#include "SDL3\SDL_timer.h"
#include "SDL3\SDL_audio.h"
#include "sound.h"
#include "chip8.h"

#define SCALE_FACTOR 4

#define TIMER_PERIOD 16666667

// todo define keymap
// probably 1234 map to 1234 and rest of them follow down

void open(char *filename, chip8State *chip8)
{
    // reset machine state
    initialize(chip8);

    // actually load in the rom file
    FILE *romfile = malloc(sizeof(FILE));
    errno_t err = fopen_s(&romfile, filename, "rb");
    if (err != 0)
    {
        printf("Failed to load file %s with error code %d\n", filename, err);
        exit(err);
    }

    // scan file for file size
    if (fseek(romfile, 0, SEEK_END) != 0)
    {
        printf("Error seeking to end of file\n");
        exit(1);
    }
    size_t filesize = ftell(romfile);
    if (filesize > 0xFFF - 0x200)
    {
        printf("File too big\n");
        exit(2);
    }

    if (fseek(romfile, 0, SEEK_SET) != 0)
    {
        printf("Error seeking to beginning of file\n");
        exit(3);
    }
    // write rom data to RAM starting at 0x200.
    size_t sizeRead = fread((chip8->ram + 0x200), sizeof(uint8_t), filesize, romfile);
    if (sizeRead < filesize)
    {
        printf("Failed to load all of the file into chip-8 ram\n");
        exit(4);
    }
    fclose(romfile);
}

int main(int argc, char **argv)
{
    chip8State *chip8 = (chip8State *)malloc(sizeof(chip8State));
    if (argc < 2)
    {
        // TODO load default rom file?
        // open("1-chip8-logo.ch8", chip8);
        // open("2-ibm-logo.ch8", chip8);
        // open("3-corax+.ch8", chip8);
        // open("4-flags.ch8", chip8);
        // open("5-quirks.ch8", chip8);
        // open("6-keypad.ch8", chip8);
        // open("7-beep.ch8", chip8);
        open("Cave.ch8", chip8);
    }
    else
    {
        char *filename = argv[1];
        open(filename, chip8);
    }
    // set up SDL window and audio.
    // TODO handle creating the audio stream! should be able to generate a something Hz square wave and play that per 60hz? maybe?
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("CHIP-8", DISPLAY_WIDTH * SCALE_FACTOR, DISPLAY_HEIGHT * SCALE_FACTOR, 0);
    if (window == NULL)
    {
        printf("Error opening SDL window: %s\n", SDL_GetError());
        exit(5);
    }
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    if (surface == NULL)
    {
        printf("Error opening SDL surface: %s\n", SDL_GetError());
        exit(6);
    }

    SDL_Surface *intermediateSurface = SDL_CreateSurface(DISPLAY_WIDTH, DISPLAY_HEIGHT, surface->format);

    // create SDL surface mapped to the chip-8 display memory!
    SDL_Surface *chip8surface = SDL_CreateSurfaceFrom(DISPLAY_WIDTH, DISPLAY_HEIGHT, SDL_PIXELFORMAT_INDEX1MSB, chip8->display, DISPLAY_WIDTH / 8);
    SDL_Palette *chip8palette = SDL_CreateSurfacePalette(chip8surface);
    chip8palette->colors[0] = (SDL_Color){0, 0, 0, 255};
    chip8palette->colors[1] = (SDL_Color){255, 255, 255, 255};

    // set up SDL audio and create our 'sample' square wave data

    SDL_AudioSpec outFormat;
    SDL_AudioDeviceID audioDeviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (audioDeviceID == -1)
    {
        printf("Error opening SDL device/stream: %s\n", SDL_GetError());
        exit(7);
    }
    if (!SDL_GetAudioDeviceFormat(audioDeviceID, &outFormat, NULL))
    {
        
        printf("Error getting SDL audio device format %s\n", SDL_GetError());
        exit(8);
    }
    outFormat.format = SDL_AUDIO_S8;
    outFormat.channels = 1;
    SDL_AudioStream *audioStream = SDL_CreateAudioStream(&outFormat, NULL);
    SDL_BindAudioStream(audioDeviceID, audioStream);
    
    int8_t *audioData = NULL;
    size_t audioDataSize = generate_square_wave(200, (size_t)outFormat.freq, &audioData);

    // set the program counter to 0x200 (where the program starts!)
    chip8->programCounter = 0x200;

    // set up a few local vars for sim state (timer/audio/etc.)
    bool running = true;
    size_t time = SDL_GetTicksNS();
    size_t nextTime = time + TIMER_PERIOD;
    bool sound = false;
    while (running)
    {
        // handle SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
            {
                uint8_t key = 0xFF;
                switch (e.key.key)
                {
                case '1':
                    key = 0x1;
                    break;
                case '2':
                    key = 0x2;
                    break;
                case '3':
                    key = 0x3;
                    break;
                case '4':
                    key = 0xC;
                    break;
                case 'q':
                    key = 0x4;
                    break;
                case 'w':
                    key = 0x5;
                    break;
                case 'e':
                    key = 0x6;
                    break;
                case 'r':
                    key = 0xD;
                    break;
                case 'a':
                    key = 0x7;
                    break;
                case 's':
                    key = 0x8;
                    break;
                case 'd':
                    key = 0x9;
                    break;
                case 'f':
                    key = 0xE;
                    break;
                case 'z':
                    key = 0xA;
                    break;
                case 'x':
                    key = 0x0;
                    break;
                case 'c':
                    key = 0xB;
                    break;
                case 'v':
                    key = 0xF;
                    break;
                }
                // update chip8 keys
                if (key != 0xFF)
                {
                    chip8->keys |= (0x1 << key);
                }
            }
            break;
            case SDL_EVENT_KEY_UP:
            {
                uint8_t key = 0xFF;
                switch (e.key.key)
                {
                case '1':
                    key = 0x1;
                    break;
                case '2':
                    key = 0x2;
                    break;
                case '3':
                    key = 0x3;
                    break;
                case '4':
                    key = 0xC;
                    break;
                case 'q':
                    key = 0x4;
                    break;
                case 'w':
                    key = 0x5;
                    break;
                case 'e':
                    key = 0x6;
                    break;
                case 'r':
                    key = 0xD;
                    break;
                case 'a':
                    key = 0x7;
                    break;
                case 's':
                    key = 0x8;
                    break;
                case 'd':
                    key = 0x9;
                    break;
                case 'f':
                    key = 0xE;
                    break;
                case 'z':
                    key = 0xA;
                    break;
                case 'x':
                    key = 0x0;
                    break;
                case 'c':
                    key = 0xB;
                    break;
                case 'v':
                    key = 0xF;
                    break;
                }
                // update chip8 keys
                if (key != 0xFF)
                {
                    chip8->keys &= ~(0x1 << key);
                }
                // if waiting for key, clear that and update the register
                if (chip8->waitingForKey)
                {
                    chip8->waitingForKey = false;
                    chip8->registers[chip8->keyRegister] = key;
                }
            }
            break;
            }
        }

        // step the emulator
        if (!chip8->waitingForKey)
        {
            step(chip8);
        }
        // handle timers
        if (SDL_GetTicksNS() >= nextTime)
        {
            if (chip8->delayTimer > 0)
            {
                chip8->delayTimer--;
            }
            if (chip8->soundTimer > 0)
            {
                chip8->soundTimer--;
            }
            nextTime += TIMER_PERIOD;
        }
        // check sound
        if (chip8->soundTimer > 0)
        {
            // start sound
            int bytesAvailable = SDL_GetAudioStreamAvailable(audioStream);
            int bytesQueued = SDL_GetAudioStreamQueued(audioStream);
            if (bytesAvailable == -1)
            {
                printf("Error getting available audio stream data %s\n", SDL_GetError());
            }
            if (bytesAvailable < (audioDataSize / 2))
            {
                if (!SDL_PutAudioStreamData(audioStream, audioData, audioDataSize))
                {
                    printf("Couldn't put data in audio stream: %s", SDL_GetError());
                }
            }
            if (!sound)
            {
                if (!SDL_ResumeAudioStreamDevice(audioStream))
                {
                    printf("Couldn't resume audio stream device: %s", SDL_GetError());
                }
                sound = true;
            }
        }

        if (sound && (chip8->soundTimer == 0))
        {
            // stop sound

            if (!SDL_PauseAudioStreamDevice(audioStream))
            {
                printf("Couldn't pause audio stream device: %s", SDL_GetError());
            }
            if (!SDL_ClearAudioStream(audioStream))
            {
                printf("Couldn't clear audio stream: %s", SDL_GetError());
            }
            sound = false;
        }
        // handle chip8 display
        if (chip8->displayChanged)
        {
            // update the surface and maybe trigger a redraw? might want to wait for multiple draws? idk
            // if (!SDL_BlitSurface())
            if (!SDL_BlitSurfaceScaled(chip8surface, NULL, surface, NULL, SDL_SCALEMODE_NEAREST))
            {
                printf("Failed to blit chip-8 surface to window surface - error: %s\n", SDL_GetError());
                exit(10);
            }
            SDL_UpdateWindowSurface(window);
        }
    }
}