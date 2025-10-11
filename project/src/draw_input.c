#include "draw_input.h"

#include <SDL2/SDL.h>
#include <stdlib.h> // necessary?
#include <math.h>

#define MIN_DIST 1.0f
#define MAX_PTS 10000

void draw_input_init(DrawInput *di){
    polyline_init(&di->line);

    di->is_drawing = 0;
    di->min_dist = MIN_DIST;
    di->max_pts = MAX_PTS;
}

void draw_input_free(DrawInput *di){
    polyline_free(&di->line);
    di->is_drawing = 0;
}

void draw_input_clear(DrawInput *di){
    polyline_clear(&di->line);
    di->is_drawing = 0;
}


// what if this fails? consider backup options
void try_add_point(DrawInput *di, float x, float y){
    if(di->max_pts && di->line.len >= di->max_pts) return; // too many points
    Vec2 p = {x, y};
    if (di->line.len > 0){
        Vec2 last = di->line.pts[di->line.len - 1];
        float add_dist = vec2_dist(last, p);
        if (add_dist < di->min_dist) return;
    }
    if(!polyline_push(&di->line, p)){
        di->is_drawing = 0;
    }
    
}

void draw_input_handling(DrawInput *di, const SDL_Event *e){

    switch(e->type){

        case SDL_MOUSEBUTTONDOWN:
            if(e->button.button == SDL_BUTTON_LEFT){
                draw_input_clear(di); // only one line at a time
                di->is_drawing = 1;
                try_add_point(di, (float)e->button.x, (float)e->button.y);
            }
            break;

        case SDL_MOUSEMOTION:
            if(di->is_drawing){
                try_add_point(di, (float)e->motion.x, (float)e->motion.y);
                // is motion or button better here? relative
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if(e->button.button == SDL_BUTTON_LEFT){
                di->is_drawing = 0;
            }
            break;

    }
}






