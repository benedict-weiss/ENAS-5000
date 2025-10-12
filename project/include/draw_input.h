#ifndef DRAW_INPUT_H
#define DRAW_INPUT_H

#include <SDL2/SDL.h>
#include "geometry.h"

#define MIN_DIST 1.5f
#define MAX_PTS 1000000

// think about this - careful with types
typedef struct {
    Polyline line;
    int is_drawing;
    float min_dist;
    size_t max_pts;
} DrawInput;


void draw_input_init(DrawInput *di);
void draw_input_free(DrawInput *di);
void draw_input_clear(DrawInput *di); // think about this - difference between pl and di

void draw_input_handling(DrawInput *di, const SDL_Event *e);
// clarify SDL_event syntax


#endif