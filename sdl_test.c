#include <unistd.h>
#define SDL2_HELPER
#include "sdl_helper.h"

int main() {
    window_size = 800;
    init_window();
    
    while (1) {
        draw_pixel(400, 400, 255, 0, 0);
        if (event_poll()) {break;}
        usleep(1000);
    }
}