#include <stdlib.h>
#include "sound.h"

size_t generate_square_wave(size_t tone_frequency, size_t output_frequency, int8_t** data)
{
    size_t data_size = output_frequency; //our sample is 1 second long.
    size_t half_period = (output_frequency/tone_frequency) / 2;
    (*data) = (int8_t*)malloc(data_size);
    if ((*data) == NULL)
    {
        exit(50);
    }
    int8_t out = 20;
    for (size_t i = 0; i < data_size; i++)
    {
        if ((i % half_period) == 0)
        {
            out = ~out; //flip the output each half-period
        }
        (*data)[i] = out;
    }
    return data_size;
}
