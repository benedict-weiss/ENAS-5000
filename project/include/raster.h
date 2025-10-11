#ifndef RASTER_H
#define RASTER_H

#include <stdlib.h>
#include <geometry.h>

#define RASTER_SIZE 512 // careful with this if changed elsewhere

// sets each pixel in raster image to 0
void raster_clear(uint8_t *img); // used uint8_t here bc it's perfect size for colour values

// converts polyline to raster image
void raster_polyline(uint8_t *img, const Polyline *pl, uint8_t val);



#endif