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

int polyline_reserve(Polyline *pl, size_t desired_cap);
int polyline_push(Polyline *pl, Vec2 p);
void polyline_clear(Polyline *pl);

int polyline_empty(Polyline *pl);

#endif