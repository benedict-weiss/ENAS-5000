#include "draw_input.h"

#include <SDL2/SDL.h>
#include <stdlib.h> // necessary?
#include <stdio.h>
#include <math.h>

void draw_input_init(DrawInput *di){
    polyline_init(&di->line);

    di->is_drawing = 0;
    di->min_dist = MIN_DIST;
    di->max_pts = MAX_PTS; // NB set to zero for unlimited


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
    if(di->max_pts && di->line.len >= di->max_pts){ // too many points
        fprintf(stderr, "[warning] polyline point cap reached (%zu)\n", di->line.len);
        di->is_drawing = 0;
        return;
    }
    Vec2 p = {x, y};
    if (di->line.len > 0){
        Vec2 last = di->line.pts[di->line.len - 1];
        float add_dist = vec2_dist(last, p);
        if (add_dist < di->min_dist) return;
    }
    if(!polyline_push(&di->line, p)){
        fprintf(stderr, "[error] out of memory adding point at %.1f,%.1f\n", x, y);
        di->is_drawing = 0;
    }
    
}

void draw_input_handling(DrawInput *di, const SDL_Event *e){

    switch(e->type){

        case SDL_MOUSEBUTTONDOWN:
            if(e->button.button == SDL_BUTTON_LEFT){
                draw_input_clear(di); // only one line at a time
                di->is_drawing = 1;
                polyline_reserve(&di->line, 500000); // precautionary
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






