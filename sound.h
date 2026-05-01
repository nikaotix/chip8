#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

size_t generate_square_wave(size_t tone_frequency, size_t output_frequency, int8_t** data);

#endif