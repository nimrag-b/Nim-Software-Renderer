#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h>
#include "time.h"

#include "renderdef.h"

typedef struct {
    HBITMAP hbm;
    void* px;
    int w, h;
} graphics_buffer;


typedef struct SR_Window{
    
    graphics_buffer mFrontBuffer;
    HWND mWindow;
    int should_quit;
    void (*get_render_target_callback)(void*, int, int);
} SR_Window;


graphics_buffer create_graphics_buffer(int w, int h)
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

void OnSize(HWND hwnd, UINT flag, int width, int height)
{
    puts("onsize");
    SR_Window* window = (SR_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    // Handle resizing
    DeleteObject(window->mFrontBuffer.hbm);
    window->mFrontBuffer = create_graphics_buffer(width, height);

    window->get_render_target_callback(window->mFrontBuffer.px, window->mFrontBuffer.w, window->mFrontBuffer.h);
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch (uMsg)
    {
    case WM_QUIT:{
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        SR_Window* window = (SR_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        window->should_quit = 1;

        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        {
            int width = LOWORD(lParam);  // Macro to get the low-order word.
            int height = HIWORD(lParam); // Macro to get the high-order word.

            // Respond to the message:
            OnSize(hwnd, (UINT)wParam, width, height);
        }
        return 0;
    case WM_PAINT:
        {
            SR_Window* window = (SR_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HDC hdc_bmp = CreateCompatibleDC(hdc);
            HGDIOBJ old_bmp = SelectObject(hdc_bmp, window->mFrontBuffer.hbm);

            BitBlt(hdc, 0, 0,  window->mFrontBuffer.w,  window->mFrontBuffer.h, hdc_bmp, 0, 0, SRCCOPY);

            SelectObject(hdc, old_bmp);
            DeleteDC(hdc_bmp);

            EndPaint(hwnd, &ps);

        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

SR_Window* SR_CreateWindow(int width, int height, void (*get_render_target)(void*, int, int)){

    printf("Creating window...\n");

    HINSTANCE hInstance = GetModuleHandle(NULL);
    // Register the window class.
    const char CLASS_NAME[]  = "Software Renderer Window Class";

    WNDCLASS wc = {0};

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowExA(
    0,                              // Optional window styles.
    CLASS_NAME,                     // Window class
    "Software Renderer",    // Window text
    WS_OVERLAPPEDWINDOW,            // Window style

    // Size and position
    CW_USEDEFAULT, CW_USEDEFAULT, width, height,

    NULL,       // Parent window    
    NULL,       // Menu
    hInstance,  // Instance handle
    NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        return NULL;
    }

    SR_Window* window = malloc(sizeof(SR_Window));
    window->mWindow = hwnd;
    window->should_quit = 0;
    window->get_render_target_callback = get_render_target;

    
    SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)window);
    
    puts("setcallback");

    ShowWindow(hwnd, 1);




    //OnSize is called before first render so we dont need to manually do this
    //_front = create_graphics_buffer(width, height);
    //get_render_target_callback(_front.px, _front.w, _front.h);



    return window;
    //ExitProcess(0);
}

void SR_DestroyWindow(SR_Window* window){

    DestroyWindow(window->mWindow);
    free(window);
}

int SR_GetMessages(SR_Window* window){
    MSG msg = {0};
    while (PeekMessage(&msg, window->mWindow,  0, 0, PM_REMOVE)) 
    { 
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
    return 0;
}

int SR_ShouldQuit(SR_Window* window){
    return window->should_quit;
}

void SR_Present(SR_Window* window){
    /*
    static LARGE_INTEGER last;
    static LONGLONG freq = 0;
    LARGE_INTEGER cur;
    QueryPerformanceCounter(&cur);
    if(freq == 0){
        LARGE_INTEGER freqI;
        QueryPerformanceFrequency(&freqI);
        freq = freqI.QuadPart;
    }

    LONGLONG ticks = cur.QuadPart - last.QuadPart;
    double dt = (double)(ticks) / freq;

    last = cur;

    printf("delta:  %lld ns\n",ticks);

    */
    InvalidateRect(window->mWindow,NULL,FALSE);
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