#include "SR.h"
#include "SR_platform.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "stdio.h"

int should_quit = 0;



float cubeverts[] = {
        -1, -1,  1, 1,0,0, //0
         1, -1,  1, 0,1,0,//1
        -1,  1,  1, 0,0,1,//2
         1,  1,  1, 1,0,0,//3
        -1, -1, -1, 1,0,0,//4
         1, -1, -1, 1,0,0,//5
        -1,  1, -1, 1,0,0,//6
         1,  1, -1, 1,0,0,//7
};

int cubeints[] = {
        //Top
        2, 6, 7,
        2, 3, 7,

        //Bottom
        0, 4, 5,
        0, 1, 5,

        //Left
        0, 2, 6,
        0, 4, 6,

        //Right
        1, 3, 7,
        1, 5, 7,

        //Front
        0, 2, 3,
        0, 1, 3,

        //Back
        4, 6, 7,
        4, 5, 7
};




void OnSize(HWND hwnd, UINT flag, int width, int height)
{
    flag = flag;
    //puts("onsize");

    SR_HWNDAttached* sr_hwnd = (SR_HWNDAttached*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    //may not be yet initalisedd the first time this is called
    if(sr_hwnd == NULL) return;
    SR_HWNDUpdateRenderTarget(sr_hwnd,width,height);
    SR_PerspFOV(sr_hwnd->pipeline,0.1f,1000,(float)width/(float)height,70);
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch (uMsg)
    {
    case WM_QUIT:{
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        should_quit = 1;

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
            SR_HWNDAttached* sr_hwnd = (SR_HWNDAttached*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SR_HWNDRender(sr_hwnd,hdc);

            

            EndPaint(hwnd, &ps);

        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND MakeWindow(int width, int height){

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


    ShowWindow(hwnd, 1);

    return hwnd;
}


void render(SR_Pipeline* pipeline){
    //printf("rendering\n");
    //clear(COLOUR_WHITE);
    SR_Clear(pipeline, (fcol4){1,1,1,1});
    SR_ClearDepthBuffer(pipeline);


    //cullmode = CULL_NONE;
    //drawmode = DRAW_TRIS;
    
    //vertexi(100,100,0);
    //vertexi(300,100,0);
    //vertexi(200,200,0);
    //draw_indexed(cubeverts,cubeints,12);


    enum SR_LAYOUT_PROP props[] = {SR_LAYOUT_POS_XYZ_F, SR_LAYOUT_COLOUR_RGB_F};
    SR_BindLayout(pipeline,props,2);

    SR_DrawIndexed(pipeline,cubeverts,cubeints,12);
}


int main(){

    HWND hwnd = MakeWindow(600,400);
    SR_Pipeline* pipeline = SR_CreatePipeline(NULL,0,0, SR_COLOUR_EMPTY);

    SR_PerspFOV(pipeline,0.1f,1000,600.0f/400.0f,70);

        
    SR_HWNDAttached* sr_hwnd = SR_AttachToHWND(hwnd, pipeline);

    SR_ToggleDepthTest(pipeline, 1);

    SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)sr_hwnd);

    long long freq = SR_TicksPerSecond();
    long long last = SR_GetTicks();
    float t = 0;
    while(!should_quit){
        long long cur = SR_GetTicks();

        double delta = (cur - last) / (double)freq;

        last = cur;



        MSG msg = {0};
        while (PeekMessage(&msg, hwnd,  0, 0, PM_REMOVE)) 
        { 
            TranslateMessage(&msg);
            DispatchMessage(&msg);

        }

        t += delta;
        mat4x4 mv = mat4transform(1,1,1, 0,0,-5,0,t,0);
        SR_BindModelview(pipeline, &mv);

        render(pipeline);


        InvalidateRect(hwnd,NULL,FALSE);

    }

    SR_DestroyPipeline(pipeline);

    return 0;
}