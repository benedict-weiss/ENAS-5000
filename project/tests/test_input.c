#include <SDL2/SDL.h>
#include "geometry.h"
#include "draw_input.h"
#include "raster.h"

int main(){

    // not checking SDL objects for failure - but working fine so far lol
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("test_input",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RASTER_SIZE, RASTER_SIZE, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    //uint8_t canvas[RASTER_SIZE * RASTER_SIZE];

    DrawInput di;
    draw_input_init(&di);

    int active = 1;

    while (active) {
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                active = 0;
            }
            draw_input_handling(&di, &e);
        }

        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255); // background colour - light gray
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); // line colour - black
        const Polyline *pl = &di.line;

        if (pl->len > 1 && pl->pts) { // should always be true
            for (size_t i = 1; i < pl->len; ++i) { // check loop conditions here
                SDL_RenderDrawLine(ren,
                    (int)pl->pts[i-1].x, (int)pl->pts[i-1].y,
                    (int)pl->pts[i].x, (int)pl->pts[i].y);
            }

        }
        SDL_RenderPresent(ren);
    }

    draw_input_free(&di);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}