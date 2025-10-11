#include <SDL2/SDL.h>
#include "geometry.h"
#include "draw_input.h"
#include "raster.h"
#include "fourier.h"

// #define RASTER_DISPLAY 1
#define PIXEL_GAP 20

int main(){

    // not checking SDL objects for failure - but working fine so far lol
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win_draw = SDL_CreateWindow("Draw Input",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RASTER_SIZE, RASTER_SIZE, 0);
    SDL_Renderer* ren_draw = SDL_CreateRenderer(win_draw, -1, SDL_RENDERER_ACCELERATED);

    SDL_Window* win_raster = SDL_CreateWindow("Raster Output",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RASTER_SIZE, RASTER_SIZE, 0);
    SDL_Renderer* ren_raster = SDL_CreateRenderer(win_raster, -1, SDL_RENDERER_ACCELERATED);

    // window positioning
    int win_x, win_y;
    int offset = (RASTER_SIZE / 2) + (PIXEL_GAP / 2);
    SDL_GetWindowPosition(win_draw, &win_x, &win_y);
    SDL_SetWindowPosition(win_draw, win_x - offset, win_y);
    SDL_SetWindowPosition(win_raster, win_x + offset, win_y);

    // windows at front
    SDL_ShowWindow(win_draw);
    SDL_ShowWindow(win_raster);
    SDL_RaiseWindow(win_draw);

    Uint32 id_draw = SDL_GetWindowID(win_draw);
    //Uint32 id_raster = SDL_GetWindowID(win_raster);

    SDL_Texture* tex_raster = SDL_CreateTexture(
        ren_raster,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        RASTER_SIZE, RASTER_SIZE);

    DrawInput di;
    draw_input_init(&di);

    static uint8_t canvas[RASTER_SIZE * RASTER_SIZE];

    int active = 1;

    while (active) {
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            switch (e.type) {
                case SDL_QUIT:
                    active = 0;
                    break;
                case SDL_WINDOWEVENT:
                    if(e.window.event == SDL_WINDOWEVENT_CLOSE){
                        active = 0;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.windowID == id_draw){
                        draw_input_handling(&di, &e);
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (e.motion.windowID == id_draw){
                        draw_input_handling(&di, &e);
                    }
                    break;

                default:
                    break;
            }
        }

        if (!active) break;

        SDL_SetRenderDrawColor(ren_draw, 128, 128, 128, 255); // background colour - light gray
        SDL_RenderClear(ren_draw);

        SDL_SetRenderDrawColor(ren_draw, 0, 0, 0, 255); // line colour - black
        const Polyline *pl = &di.line;

        if (pl->len > 1 && pl->pts) { // should always be true
            for (size_t i = 1; i < pl->len; ++i) { // check loop conditions here
                SDL_RenderDrawLine(ren_draw,
                    (int)pl->pts[i-1].x, (int)pl->pts[i-1].y,
                    (int)pl->pts[i].x, (int)pl->pts[i].y);
            }

        }
        SDL_RenderPresent(ren_draw);

        raster_clear(canvas);
        raster_polyline(canvas, &di.line, 255); // white line colour

        // fourier_spectrum(canvas, RASTER_SIZE, RASTER_SIZE);

        void *pixels = NULL; // raw ptr
        uint8_t *dest = NULL; // writing into here
        int pitch = 0;

        if (SDL_LockTexture(tex_raster, NULL, &pixels, &pitch)==0){;
            dest = (uint8_t *)pixels;
            for (int y = 0; y < RASTER_SIZE; y++){
                for (int x = 0; x < RASTER_SIZE; x++){
                    uint8_t v = canvas[y * RASTER_SIZE + x];
                    dest[y * pitch + x * 3 + 0] = v;
                    dest[y * pitch + x * 3 + 1] = v;
                    dest[y * pitch + x * 3 + 2] = v;
                }
            }
            SDL_UnlockTexture(tex_raster);
        }


        SDL_SetRenderDrawColor(ren_raster, 0, 0, 0, 255); // background colour - black
        SDL_RenderClear(ren_raster);
        SDL_RenderCopy(ren_raster, tex_raster, NULL, NULL);
        if(di.is_drawing == 0){
            SDL_RenderPresent(ren_raster);
        }

    }

    draw_input_free(&di);

    SDL_DestroyTexture(tex_raster);
    SDL_DestroyRenderer(ren_raster);
    SDL_DestroyWindow(win_raster);

    SDL_DestroyRenderer(ren_draw);
    SDL_DestroyWindow(win_draw);
    SDL_Quit();
    return 0;
}