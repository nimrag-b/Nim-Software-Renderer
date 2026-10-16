/*
 * This file contains platform specific helper functions. These functions will only work if a platform helper backend is supplied.
 * 
 */

#ifndef _SR_PLATFORM_H_
#define _SR_PLATFORM_H_

#include "SR.h"
#include "SR_internal.h"

#ifdef __cplusplus
extern "C" {
#endif


SR_DLLFUNC long long SR_GetTicks();
SR_DLLFUNC long long SR_TicksPerSecond();


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


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


SR_DLLFUNC void SR_HWNDUpdateRenderTarget(SR_HWNDAttached* sr_hwnd, int width, int height);
SR_DLLFUNC void SR_HWNDRender(SR_HWNDAttached* sr_hwnd, HDC hdc);

SR_DLLFUNC SR_HWNDAttached* SR_AttachToHWND(HWND hwnd, SR_Pipeline* pipeline);
SR_DLLFUNC void SR_DetachFromHWND(SR_HWNDAttached* sr_hwnd);

#endif //_WIN32


#ifdef __cplusplus
}
#endif //__cplusplus


#endif //_SR_PLATFORM_H_
