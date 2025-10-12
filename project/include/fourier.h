#ifndef FOURIER_H
#define FOURIER_H

#include <stdlib.h>
#include <stdint.h>

// NB going to start with FFT but may implement naive dft2d etc later

int fourier_spectrum(uint8_t *canvas, size_t width, size_t height);

#endif