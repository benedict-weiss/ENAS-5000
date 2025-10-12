#include "geometry.h"
#include "draw_input.h" // this might not be great

#include <stdlib.h> // necessary?
#include <math.h>

#define START_CAP 128

float vec2_dist(Vec2 a, Vec2 b){
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dist = sqrtf(dx*dx + dy*dy);
    return dist;
}

Vec2 vec2_add(Vec2 a, Vec2 b){
    float new_x = a.x + b.x;
    float new_y = a.y + b.y;
    Vec2 new_vec = (Vec2){new_x, new_y};
    return new_vec;
}

Vec2 vec2_sub(Vec2 a, Vec2 b){
    float new_x = a.x - b.x;
    float new_y = a.y - b.y;
    Vec2 new_vec = (Vec2){new_x, new_y};
    return new_vec;
}

Vec2 vec2_scale(Vec2 a, float s){
    float new_x = a.x * s;
    float new_y = a.y * s;
    Vec2 new_vec = (Vec2){new_x, new_y};
    return new_vec;
}

void polyline_init(Polyline *pl){
    pl->pts = NULL;
    pl->len = 0;
    pl->cap = 0;
}

void polyline_free(Polyline *pl){
    free(pl->pts); // NB safe even if NULL ptr freed
    pl->pts = NULL;
    pl->len = 0;
    pl->cap = 0;
}

int polyline_reserve(Polyline *pl, size_t desired_cap){
    size_t hard_cap = MAX_PTS; // necessary?
    if(hard_cap && desired_cap > hard_cap) {
        desired_cap = hard_cap;
    }

    if(desired_cap <= pl->cap) {
        if (hard_cap && pl->cap >= hard_cap && pl->len >= pl->cap) return 0;
        return 1;
    }

    size_t new_cap;
    if (pl->cap != 0){
        new_cap = pl->cap;
    } else {
        new_cap = START_CAP;
    }

    size_t max_cap = SIZE_MAX / sizeof(Vec2);
    if (desired_cap > max_cap) return 0;

    while (new_cap < desired_cap){
        if (new_cap > max_cap / 2){ // prevents overflow of size_t
            new_cap = desired_cap;
            break;
        }
        new_cap *= 2;
    }
    
    void *p = realloc(pl->pts, new_cap * sizeof(Vec2));

    if(!p) return 0; // malloc failure

    pl->pts = (Vec2*)p;
    pl->cap = new_cap;

    return 1;

}

int polyline_push(Polyline *pl, Vec2 p){
    if (pl->len == pl->cap){
        if (!polyline_reserve(pl, pl->len + 1)){
            return 0;
        }
        if (pl->len == pl->cap) return 0; // overkill?
    }

    pl->pts[pl->len++] = p;
    return 1;
}

void polyline_clear(Polyline *pl){
    pl->len = 0;
}

int polyline_empty(Polyline *pl){
    return pl->len == 0;
}
