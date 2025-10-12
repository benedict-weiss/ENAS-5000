#ifndef FOURIER_H
#define FOURIER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// NB going to start with FFT but may implement naive dft2d etc later

int fourier_1d(uint8_t *canvas, size_t width, size_t height);

int fourier_2d(uint8_t *canvas, size_t width, size_t height);

#endif