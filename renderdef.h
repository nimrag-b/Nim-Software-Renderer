#ifndef _RENDER_DEF_H
#define _RENDER_DEF_H

typedef struct{
    char r, g, b, a;
} colour;




typedef struct SR_Window SR_Window;


void* get_gfx_buffer(int* w, int* h);

SR_Window* SR_CreateWindow(int width, int height, void (*get_render_target)(void*, int, int));

void SR_DestroyWindow(SR_Window* window);

int SR_GetMessages(SR_Window* window);
void SR_Present(SR_Window* window);
int SR_ShouldQuit(SR_Window* window);

long long SR_GetTicks();
long long SR_TicksPerSecond();
#endif