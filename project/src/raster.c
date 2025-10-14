#include "raster.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>


// basic implementation of Bresenham line algorithm found online (https://gist.github.com/bert/1085538)
// rasterises line and sets pixels to val
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
    memset(img, 0, RASTER_SIZE * RASTER_SIZE * sizeof(uint8_t)); // NB uint8_t should be a byte
}

void raster_polyline(uint8_t *img, const Polyline *pl, uint8_t val){
    if (!pl || !pl->pts || pl->len < 2) return;

    for (size_t i = 1; i < pl->len; ++i){ // check bounds here
        int x0 = (int)lroundf(pl->pts[i-1].x);
        int y0 = (int)lroundf(pl->pts[i-1].y);
        int x1 = (int)lroundf(pl->pts[i].x);
        int y1 = (int)lroundf(pl->pts[i].y);
        raster_line(img, x0, y0, x1, y1, val);
    }
}

void raster_closed_polyline(uint8_t *img, const Polyline *pl, uint8_t val) {
    if (!pl || !pl->pts || pl->len < 2) return;

    // start from last to first
    size_t n = pl->len;
    int x_prev = (int)lroundf(pl->pts[n-1].x);
    int y_prev = (int)lroundf(pl->pts[n-1].y);

    for (size_t i = 0; i < n; ++i) {
        int x_curr = (int)lroundf(pl->pts[i].x);
        int y_curr = (int)lroundf(pl->pts[i].y);

        raster_line(img, x_prev, y_prev, x_curr, y_curr, val);
        
        x_prev = x_curr;
        y_prev = y_curr;
    }
}







