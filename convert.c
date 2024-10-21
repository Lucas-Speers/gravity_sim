#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void expect(void *p) {
    if (p == NULL) {
        printf("Bad alloc\n");
        exit(1);
    }
}

int main() {
    char *conversion_command = calloc(255, sizeof(char));
    expect(conversion_command);
    
    char *remove_file = calloc(255, sizeof(char));
    expect(remove_file);
    
    int index = 0;
    while (1) {
        // sprintf(conversion_command, "magick %d.ppm %d.png", index, index);
        sprintf(conversion_command, "convert %d.ppm %d.png", index, index);
        printf("%s\n", conversion_command);
        if (system(conversion_command)) {
            if (index == 0) {
                exit(1); 
            }
            break;
        }
        index++;
    }
    free(conversion_command);
    free(remove_file);
    
    system("rm *.ppm");
    if (system("ffmpeg -f image2 -framerate 30 -i %01d.png -vcodec libx264 -crf 22 video.mp4") == 0) {
        system("rm *.png");
    }
}