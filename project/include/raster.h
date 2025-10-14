#ifndef RASTER_H
#define RASTER_H

#include "fourier.h"

#include <stdlib.h>
#include <stdint.h>
#include "geometry.h"


#define RASTER_SIZE 512 // maybe have this be cli? make sure it's a power of two

void raster_line(uint8_t *img, int x0, int y0, int x1, int y1, uint8_t val);

// sets each pixel in raster image to 0
void raster_clear(uint8_t *img); // used uint8_t here bc it's perfect size for colour values

// converts polyline to raster image
void raster_polyline(uint8_t *img, const Polyline *pl, uint8_t val);

// makes sure raster image is a loop
void raster_closed_polyline(uint8_t *img, const Polyline *pl, uint8_t val);

void raster_closed_line_from_pts(uint8_t *img, const Pt *pts, size_t N, uint8_t v);

#endif