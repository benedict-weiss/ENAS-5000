#include "raster.h"

#include <stdlib.h>
#include <math.h>


// basic implementation of Bresenham line algorithm found online (https://gist.github.com/bert/1085538)
void raster_line(uint8_t *img, int x0, int y0, int x1, int y1, uint8_t val){
    int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = dx + dy, e2;

    while (1) {
        if (x0 >= 0 && x0 < RASTER_SIZE && y0 >= 0 && y0 < RASTER_SIZE){
            img[y0 * RASTER_SIZE + x0] = val;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


void raster_clear(uint8_t *img){
    memset(img, 0, RASTER_SIZE * RASTER_SIZE);
}

void raster_polyline(uint8_t *img, Polyline *pl, uint8_t val){
    if (!pl || pl->len < 2) return;

    for (size_t i = 1; i < pl->len; ++i){ // check bounds
        int x0 = (int)roundf(pl->pts[i-1].x);
        int y0 = (int)roundf(pl->pts[i-1].y);
        int x1 = (int)roundf(pl->pts[i].x);
        int y1 = (int)roundf(pl->pts[i].y);
        raster_line(img, x0, y0, x1, y1, val);
    }
}








