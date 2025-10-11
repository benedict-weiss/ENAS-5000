#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdlib.h>

// be careful with types here in general

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
    Vec2* pts;
    size_t len;
    size_t cap;
} Polyline;

float vec2_dist(Vec2 a, Vec2 b);
Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_sub(Vec2 a, Vec2 b);
Vec2 vec2_scale(Vec2 a, float s);

void polyline_init(Polyline *pl);
void polyline_free(Polyline *pl);

// reserves space on heap for polyline with size desired_cap
// returns 1 if allocated successfully or there's already capacity
// returns 0 in case of malloc failure
int polyline_reserve(Polyline *pl, size_t desired_cap);

// adds Vec2 point to polyline
// returns 1 if successful
// returns 0 if not enough capacity and the reserve function fails
int polyline_push(Polyline *pl, Vec2 p);

// need to think whether this is sufficient
void polyline_clear(Polyline *pl);

// returns 1 if polyline has no pts, otherwise 0 if non-empty
int polyline_empty(Polyline *pl);

#endif


// NOTES:
// polyline_clear just sets len to 0 - make sure use this variable to check elsewhere