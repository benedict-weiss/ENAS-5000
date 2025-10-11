#ifndef DRAW_INPUT_H
#define DRAW_INPUT_H

#include "geometry.h"


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

void draw_input_handling(DrawInput *di, const void *event_ptr);
// clarify SDL_event syntax


#endif