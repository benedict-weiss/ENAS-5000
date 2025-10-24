#ifndef FOURIER_H
#define FOURIER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

// careful with M_PI

#include "geometry.h"

typedef struct {
    double x;
    double y;
} Pt;

int fourier_1d(uint8_t *canvas, size_t width, size_t height, int num_terms);

int fourier_2d_from_pl(uint8_t *canvas, size_t width, size_t height, int num_terms, const Polyline *pl);

#endif