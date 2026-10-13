#ifndef _SR_PLATFORM_H_
#define _SR_PLATFORM_H_

#include "SR.h"

#ifdef _WIN32

#include <windef.h>


typedef struct {
    HBITMAP hbm;
    void* px;
    int w, h;
} graphics_buffer;

typedef struct SR_HWNDAttached{
    SR_Pipeline* pipeline;
    graphics_buffer mFrontBuffer;
    HWND mWindow;

} SR_HWNDAttached;


void SR_HWNDUpdateRenderTarget(SR_HWNDAttached* sr_hwnd, int width, int height);
void SR_HWNDRender(SR_HWNDAttached* sr_hwnd, HDC hdc);

SR_HWNDAttached* SR_AttachToHWND(HWND hwnd, SR_Pipeline* pipeline);
void SR_DetachFromHWND(SR_HWNDAttached* sr_hwnd);
#endif
#endif