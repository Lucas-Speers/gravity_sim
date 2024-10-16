#include <SDL2/SDL.h>

void init_window();
void draw_pixel(int x, int y, int r, int g, int b);
int event_poll();

#ifdef SDL2_HELPER

SDL_Window *window;
SDL_Renderer *renderer;

int window_size;

void init_window() {
    SDL_CreateWindowAndRenderer(window_size, window_size, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
}

void draw_pixel(int x, int y, int r, int g, int b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderDrawPoint(renderer, x, y);
    SDL_RenderPresent(renderer);
}

int event_poll() {
    SDL_Event event;
    if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
        return 1;
    return 0;
}

#endif