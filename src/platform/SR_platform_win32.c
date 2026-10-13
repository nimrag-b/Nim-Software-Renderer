#include "SR_platform.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



graphics_buffer SR_HWNDCreateGraphicsBuffer(int w, int h)
{
    HDC hdcScreen = GetDC(NULL);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    graphics_buffer gb;
    gb.w = w;
    gb.h = h;
    gb.hbm = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)(&gb.px), NULL, 0);


    ReleaseDC(NULL, hdcScreen);
    return gb;
}

void SR_HWNDUpdateRenderTarget(SR_HWNDAttached* sr_hwnd, int width, int height){

    if(sr_hwnd == NULL) return;
    DeleteObject(sr_hwnd->mFrontBuffer.hbm);
    sr_hwnd->mFrontBuffer = SR_HWNDCreateGraphicsBuffer(width, height);
    SR_UpdateRenderTarget(sr_hwnd->pipeline, sr_hwnd->mFrontBuffer.px, sr_hwnd->mFrontBuffer.w, sr_hwnd->mFrontBuffer.h);
}

void SR_HWNDRender(SR_HWNDAttached* sr_hwnd, HDC hdc){
    
    HDC hdc_bmp = CreateCompatibleDC(hdc);
    HGDIOBJ old_bmp = SelectObject(hdc_bmp, sr_hwnd->mFrontBuffer.hbm);

    BitBlt(hdc, 0, 0,  sr_hwnd->mFrontBuffer.w,  sr_hwnd->mFrontBuffer.h, hdc_bmp, 0, 0, SRCCOPY);

    SelectObject(hdc, old_bmp);
    DeleteDC(hdc_bmp);
}

SR_HWNDAttached* SR_AttachToHWND(HWND hwnd, SR_Pipeline* pipeline){
    SR_HWNDAttached* sr_hwnd = malloc(sizeof(SR_HWNDAttached));
    sr_hwnd->mWindow = hwnd;
    sr_hwnd->pipeline = pipeline;
    RECT rect;
    GetWindowRect(hwnd, &rect);
    SR_HWNDUpdateRenderTarget(sr_hwnd,rect.right - rect.left, rect.bottom - rect.top);

    return sr_hwnd;
}

void SR_DetachFromHWND(SR_HWNDAttached* sr_hwnd){

    DeleteObject(sr_hwnd->mFrontBuffer.hbm);
    free(sr_hwnd);
}


long long SR_GetTicks(){
    LARGE_INTEGER cur;
    QueryPerformanceCounter(&cur);
    return cur.QuadPart;
}
long long SR_TicksPerSecond(){
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
}