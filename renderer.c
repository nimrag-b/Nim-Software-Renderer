#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <math.h>

#include "renderdef.h"

#include <time.h>

#ifndef M_PI
#define M_PI 3.14159
#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define CLAMP(v, a, b) (MAX(MIN(v,b),a))

//#define M_PI 3.14159

#define COLOUR_WHITE (colour){255,255,255,255}


static float* depth_buffer = 0;

static colour* screen = 0;
static int scr_w = 0;
static int scr_h = 0;

//STATE MACHINE

enum CULLMODE{
    CULL_NONE = 0,
    CULL_FRONT = 1,
    CULL_BACK = -1,
};
int cullmode = CULL_NONE;

enum DRAWMODE{
    DRAW_TRIS,
    DRAW_POINTS,
    DRAW_LINES
};
int drawmode = DRAW_TRIS;


enum LAYOUT_TYPE{
    LAYOUT_TYPE_POSITION,
    LAYOUT_TYPE_TEXCOORD,
    LAYOUT_TYPE_COLOUR,
};
enum LAYOUT_FORMAT{
    LAYOUT_FORMAT_FLOAT,
    LAYOUT_FORMAT_FLOAT2,
    LAYOUT_FORMAT_FLOAT3,
};

typedef struct vert_layout_prop
{
    int offset;
    int stride;
    int type;
    int format;
} vert_layout_prop;

vert_layout_prop* cur_props = NULL;
int prop_count = 0;

mat4x4 persp;
mat4x4 modelview;

//STATE MACHINE

void set_modelview(mat4x4* matrix){
    modelview = *matrix;
}


void set_persp_fov(float near, float far, float aspect, float FOV){
    float e = 1.0f / tanf((FOV * (M_PI / 180.0f))/2.0f); 

    printf("e= %f\n",e);

    persp = (mat4x4){
        e/aspect,               0,                      0,      0,
        0,                      e,                      0,      0,
        0,                      0,                      (far+near)/(near-far),          (2*far*near)/(near-far),
        0,                      0,                      -1,                             0,
    };
}

void set_persp(float near, float far, float left, float right, float top, float bottom){

    persp = (mat4x4){
        (2*near)/(right-left),  0,                      (right+left)/(right-left),      0,
        0,                      (2*near)/(top-bottom),  (top+bottom)/(top-bottom),      0,
        0,                      0,                      (far+near)/(near-far),          (2*far*near)/(near-far),
        0,                      0,                      -1,                             0,
    };
}

mat4x4 mat4identity(){
    mat4x4 m = {0};
    m.m00 = 1;
    m.m11 = 1;
    m.m22 = 1;
    m.m33 = 1;
    return m;
}

mat4x4 mat4translate(float x, float y, float z){
    mat4x4 m = {0};
    m.m00 = 1;
    m.m11 = 1;
    m.m22 = 1;
    m.m33 = 1;

    m.m03 = x;
    m.m13 = y;
    m.m23 = z;

    return m;
}
mat4x4 mat4scale(float x, float y, float z){
    mat4x4 m = {0};
    m.m00 = x;
    m.m11 = y;
    m.m22 = z;
    m.m33 = 1;

    return m;
}

mat4x4 mat4rotatex(float t){
    mat4x4 m = {0};
    m.m00 = 1;
    m.m11 = cosf(t);
    m.m12 = -sinf(t);
    m.m21 = sinf(t);
    m.m22 = cosf(t);
    m.m33 = 1;

    return m;
}
mat4x4 mat4rotatey(float t){
    mat4x4 m = {0};
    m.m00 = cosf(t);
    m.m02 = sinf(t);
    m.m11 = 1;
    m.m20 = -sinf(t);
    m.m22 = cosf(t);
    m.m33 = 1;

    return m;
}
mat4x4 mat4rotatez(float t){
    mat4x4 m = {0};

    m.m00 = cosf(t);
    m.m01 = -sinf(t);
    m.m10 = sinf(t);
    m.m11 = cosf(t);
    m.m22 = 1;
    m.m33 = 1;

    return m;
}


mat4x4 mat4transpose(mat4x4* m){
    mat4x4 r;
    r.m00 = m->m00;
    r.m01 = m->m10;
    r.m02 = m->m20;
    r.m03 = m->m30;

    r.m10 = m->m01;
    r.m11 = m->m11;
    r.m12 = m->m21;
    r.m13 = m->m31;

    r.m20 = m->m02;
    r.m21 = m->m12;
    r.m22 = m->m22;
    r.m23 = m->m32;

    r.m30 = m->m03;
    r.m31 = m->m13;
    r.m32 = m->m23;
    r.m33 = m->m33;
}

mat4x4 mat4mul4(mat4x4* b, mat4x4* a){
    mat4x4 m;
    m.m01 = (a->m00 * b->m01) + (a->m01 * b->m11) + (a->m02 * b->m21) + (a->m03 * b->m31);
    m.m00 = (a->m00 * b->m00) + (a->m01 * b->m10) + (a->m02 * b->m20) + (a->m03 * b->m30);
    m.m02 = (a->m00 * b->m02) + (a->m01 * b->m12) + (a->m02 * b->m22) + (a->m03 * b->m32);
    m.m03 = (a->m00 * b->m03) + (a->m01 * b->m13) + (a->m02 * b->m23) + (a->m03 * b->m33);
    m.m10 = (a->m10 * b->m00) + (a->m11 * b->m10) + (a->m12 * b->m20) + (a->m13 * b->m30);
    m.m11 = (a->m10 * b->m01) + (a->m11 * b->m11) + (a->m12 * b->m21) + (a->m13 * b->m31);
    m.m12 = (a->m10 * b->m02) + (a->m11 * b->m12) + (a->m12 * b->m22) + (a->m13 * b->m32);
    m.m13 = (a->m10 * b->m03) + (a->m11 * b->m13) + (a->m12 * b->m23) + (a->m13 * b->m33);
    m.m20 = (a->m20 * b->m00) + (a->m21 * b->m10) + (a->m22 * b->m20) + (a->m23 * b->m30);
    m.m21 = (a->m20 * b->m01) + (a->m21 * b->m11) + (a->m22 * b->m21) + (a->m23 * b->m31);
    m.m22 = (a->m20 * b->m02) + (a->m21 * b->m12) + (a->m22 * b->m22) + (a->m23 * b->m32);
    m.m23 = (a->m20 * b->m03) + (a->m21 * b->m13) + (a->m22 * b->m23) + (a->m23 * b->m33);
    m.m30 = (a->m30 * b->m00) + (a->m31 * b->m10) + (a->m32 * b->m20) + (a->m33 * b->m30);
    m.m31 = (a->m30 * b->m01) + (a->m31 * b->m11) + (a->m32 * b->m21) + (a->m33 * b->m31);
    m.m32 = (a->m30 * b->m02) + (a->m31 * b->m12) + (a->m32 * b->m22) + (a->m33 * b->m32);
    m.m33 = (a->m30 * b->m03) + (a->m31 * b->m13) + (a->m32 * b->m23) + (a->m33 * b->m33);
    return m;
}

float4 mat4mul(mat4x4* mat, float4 v){
    float4 res;
    res.x =  (mat->m00 * v.x) + (mat->m01 * v.y) + (mat->m02 * v.z)+ (mat->m03 * v.w);
    res.y =  (mat->m10 * v.x) + (mat->m11 * v.y) + (mat->m12 * v.z)+ (mat->m13 * v.w);
    res.z =  (mat->m20 * v.x) + (mat->m21 * v.y) + (mat->m22 * v.z)+ (mat->m23 * v.w);
    res.w =  (mat->m30 * v.x) + (mat->m31 * v.y) + (mat->m32 * v.z)+ (mat->m33 * v.w);
    return res;
}



mat4x4 mat4transform(float scaleX, float scaleY, float scaleZ, float transX, float transY, float transZ, float rotX, float rotY, float rotZ){

    mat4x4 scale = mat4scale(scaleX, scaleY, scaleZ);
    mat4x4 translation = mat4translate(transX, transY, transZ);
    mat4x4 rotateX = mat4rotatex(rotX);
    mat4x4 rotateY = mat4rotatey(rotY);
    mat4x4 rotateZ = mat4rotatez(rotZ);

    mat4x4 m = mat4mul4(&scale, &rotateY);
    m = mat4mul4(&m, &rotateX);
    m = mat4mul4(&m, &rotateZ);
    m = mat4mul4(&m, &translation);
    return m;
}


float4 vert_shade(float x, float y, float z){
    float4 p = mat4mul(&modelview, (float4){x,y,z,1.0f});
    p = mat4mul(&persp,p);
    return p;
}

colour pix_shade(float r, float g, float b){
    return (colour){CLAMP(b,0,1) * 255,CLAMP(g,0,1) * 255, CLAMP(r,0,1) * 255, 255};
}

void clear(colour c){
    int v = c.r + (c.g << 8) + (c.b << 16) + (c.a << 24);

    unsigned long long count = scr_w * scr_h;

    int* ptr = screen;

    for (unsigned long long i = 0; i < count; i++)
    {
        ptr[i] = v;
    }
    
}

void clear_depth_buffer(){
    int c = scr_w * scr_h;
    //printf("clearing (%d)\n",c);
    for (size_t i = 0; i < c; i++)
    {
        depth_buffer[i] = 1000000.0f;
    }
    //puts("cleared");
    //memset(depth_buffer,1000000, scr_w * scr_h * sizeof(depth_buffer[0]));
}

void draw_point(int x, int y, float r, float g, float b){
            
    if (x >= 0 && x < scr_w && y >= 0 && y < scr_h){
        screen[x + (y * scr_w)] = pix_shade(r,g,b);
    }
}

#define OPENGL_COORDS

#ifdef OPENGL_COORDS // -1 -> +1
#define COORD_NORMALISE(f) ((f+1)/2.0f)
#else // 0 -> 1
#define COORD_NORMALISE(f) (f)
#endif

#define NORMALISE_COORD_X(f) ((int)roundf(COORD_NORMALISE(f) * scr_w))
#define NORMALISE_COORD_Y(f) ((int)roundf(COORD_NORMALISE(f) * scr_h))

void line_low(int x1, int y1, int x2, int y2){
    int dx = x2 - x1;
    int dy = y2 - y1;
    int yi = 1;
    if(dy < 0){
        yi = -1;
        dy = -dy;
    }
    int D = (2 * dy) - dx;
    int y = y1;

    for (int x = x1; x < x2; ++x)
    {

        draw_point(x, y,0,0,0);
        if(D > 0){
            y += yi;
            D = D + (2 * (dy - dx));
        }
        else{
            D = D + 2*dy;
        }
    }

}

void line_high(int x1, int y1, int x2, int y2){
    int dx = x2 - x1;
    int dy = y2 - y1;
    int xi = 1;
    if(dx < 0){
        xi = -1;
        dx = -dx;
    }
    int D = (2 * dx) - dy;
    int x = x1;

    for (int y = y1; y < y2; ++y)
    {

        draw_point(x, y,0,0,0);
        if(D > 0){
            x += xi;
            D = D + (2 * (dx - dy));
        }
        else{
            D = D + 2*dx;
        }
    }

}

void bresenham(int x1, int y1, int x2, int y2){

    if(abs(y2 - y1) < abs(x2 - x1)){
        if(x1 > x2){
            line_low(x2,y2,x1,y1);
        }
        else{
            line_low(x1,y1,x2,y2);
        }
    }
    else{
        if(y1 > y2){
            line_high(x2,y2,x1,y1);
        }
        else{
            line_high(x1,y1,x2,y2);
        }
    }
}

void rasterize_lines( int x1, int y1, int x2, int y2, int x3, int y3){
    bresenham(x1,y1,x2,y2);
    bresenham(x3,y3,x2,y2);
    bresenham(x1,y1,x3,y3);
}

void rasterize_tris(
    int x1, int y1, float d1, float r1, float g1, float b1, 
    int x2, int y2, float d2, float r2, float g2, float b2, 
    int x3, int y3, float d3, float r3, float g3, float b3){

    int min_x = MIN(MIN(x1, x2), x3);
    int max_x = MAX(MAX(x1, x2), x3);
    int min_y = MIN(MIN(y1, y2), y3);
    int max_y = MAX(MAX(y1, y2), y3);

    float f = (x1 *(y2 - y3)) + (x2 *(y3 - y1)) + (x3 *(y1 - y2));

    f = 1/f; //invert

    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;

    m00 = (x2*y3) - (x3*y2);
    m10 = (x3*y1) - (x1*y3);
    m20 = (x1*y2) - (x2*y1);

    m01 = y2 - y3;
    m11 = y3 - y1;
    m21 = y1 - y2;
    
    m02 = x3 - x2;
    m12 = x1 - x3;
    m22 = x2 - x1;

    //(B - A) x (C - A)
    //calculate the z property of the cross product to get the winding of the triangle
    //x and y cancel out to 0 when z is 0 (ie a flat triangle)
    

    if(cullmode != 0){ //cullmode of zero means dont cull
        int winding = (-(y2 - y1) * (x3 - x1)) + ((x2 - x1) * (y3 - y1));

        //printf("Winding = %d\n",winding);
        //do some bitwise trickery to determine wether to cull or not
        if((winding ^ cullmode) >= 0){
            return;
        }
    }





    for (int j = min_y; j < max_y; j++)
    {
        for (int i = min_x; i < max_x; i++)
        {
            float bx =  ((m00 * 1) + (m01 * i) + (m02 * j)) * f;
            float by =  ((m10 * 1) + (m11 * i) + (m12 * j)) * f;
            float bz =  ((m20 * 1) + (m21 * i) + (m22 * j)) * f;

            //in triangle
            if(bx > 0 && by > 0 && bz > 0){

                float d = (d1 * bx) + (d2 * by) + (d3 * bz);
                //printf("[%f] <- %f\n",depth_buffer[i + j*scr_w], d);
                if(d > depth_buffer[i + j*scr_w]){
                    continue;
                }
                depth_buffer[i + j*scr_w] = d;
                float r = (r1 * bx) + (r2 * by) + (r3 * bz);
                float g = (g1 * bx) + (g2 * by) + (g3 * bz);
                float b = (b1 * bx) + (b2 * by) + (b3 * bz);

                draw_point(i,j, r, g, b);
            }
        }
    }
    
}

void rasterize_points(int x1, int y1, int x2, int y2, int x3, int y3){
    draw_point(x1,y1,0,0,0);
    draw_point(x2,y2,0,0,0);
    draw_point(x3,y3,0,0,0);
}

void rasterize(int x1, int y1, float d1, float r1, float g1, float b1, int x2, int y2, float d2, float r2, float g2, float b2, int x3, int y3, float d3, float r3, float g3, float b3){

    switch (drawmode)
    {
    case DRAW_LINES:
        rasterize_lines(
            x1,y1,
            x2,y2,
            x3,y3);
        break;
    case DRAW_TRIS:
        rasterize_tris(
            x1,y1, d1, r1,g1,b1,
            x2,y2, d2, r2,g2,b2,
            x3,y3, d3, r3,g3,b3);
        break;
    case DRAW_POINTS:
        rasterize_points(
            x1,y1,
            x2,y2,
            x3,y3);
        break;
    default:
        break;
    }
}




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

void bind_layout(vert_layout_prop* props, int count){
    cur_props = props;
    prop_count = count;
}





void draw_indexed(float* vertices, int* indices, unsigned long long count){

#define FORMAT_LENGTH 6
    int c = 0;

    count = count * 3;
    for (unsigned long long i = 0; i < count; i += 3)
    {
        
        int i1 = (indices[(i)+0] * FORMAT_LENGTH);
        int i2 = (indices[(i)+1] * FORMAT_LENGTH);
        int i3 = (indices[(i)+2] * FORMAT_LENGTH);



        float x1 = vertices[i1 + 0];
        float y1 = vertices[i1 + 1];
        float z1 = vertices[i1 + 2];
        float r1 = vertices[i1 + 3];
        float g1 = vertices[i1 + 4];
        float b1 = vertices[i1 + 5];

        float x2 = vertices[i2 + 0];
        float y2 = vertices[i2 + 1];
        float z2 = vertices[i2 + 2];
        float r2 = vertices[i2 + 3];
        float g2 = vertices[i2 + 4];
        float b2 = vertices[i2 + 5];

        float x3 = vertices[i3 + 0];
        float y3 = vertices[i3 + 1];
        float z3 = vertices[i3 + 2];
        float r3 = vertices[i3 + 3];
        float g3 = vertices[i3 + 4];
        float b3 = vertices[i3 + 5];


        //printf("tri = (%.1f,%.1f,%.1f) (%.1f,%.1f,%.1f) (%.1f,%.1f,%.1f)\n",x1,y1,z1,x2,y2,z2,x3,y3,z3);

        //vertex shader

        

        float4 p1 = vert_shade(x1,y1,z1);
        float4 p2 = vert_shade(x2,y2,z2);
        float4 p3 = vert_shade(x3,y3,z3);

        //printf("(%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f)\n", x1,y1,z1,1.0f, p1.x,p1.y,p1.z,p1.w, p11.x, p11.y, p11.z, p11.w, p11.x/p11.z,p11.y/p11.z);

        rasterize(
            NORMALISE_COORD_X(p1.x/p1.z), NORMALISE_COORD_Y(p1.y/p1.z), p1.w, r1,g1,b1,
            NORMALISE_COORD_X(p2.x/p2.z), NORMALISE_COORD_Y(p2.y/p2.z), p2.w, r2,g2,b2,
            NORMALISE_COORD_X(p3.x/p3.z), NORMALISE_COORD_Y(p3.y/p3.z), p3.w, r3,g3,b3
        );
        c++;
    }

    //printf("rendered %d tris\n",c);
    
}

void set_render_target(void* data, int w, int h){
    screen = data;
    scr_w = w;
    scr_h = h;

    if(depth_buffer){
        free(depth_buffer);
    }

    depth_buffer = calloc(1, scr_w * scr_h * sizeof(depth_buffer[0]));
    float aspect = (float)w / (float)h;
    printf("aspect=%f\n",aspect);
    set_persp_fov(0.1f,1000, aspect, 90);

    printf("setting target %p (%d, %d)\n",data, w, h);
}

void render(){
    //printf("rendering\n");
    clear(COLOUR_WHITE);
    clear_depth_buffer();
    cullmode = CULL_NONE;
    drawmode = DRAW_TRIS;
    //vertexi(100,100,0);
    //vertexi(300,100,0);
    //vertexi(200,200,0);
    draw_indexed(cubeverts,cubeints,12);
}

int main(){

    printf("Hello World!\n");

  

    SR_Window* window = SR_CreateWindow(600,400,set_render_target);

    long long freq = SR_TicksPerSecond();
    long long last = SR_GetTicks();
    if(window == NULL) return -1;
    float t = 0;
    while(!SR_ShouldQuit(window)){
        long long cur = SR_GetTicks();

        double delta = (cur - last) / (double)freq;

        last = cur;

        SR_GetMessages(window);

        


        t += delta;

        /*
        mat4x4 rotation = mat4rotatey(0);
        mat4x4 rot2 = mat4rotatex(t);
        rotation = mat4mul4(&rotation, &rot2);
        mat4x4 translation = mat4translate(0,0,-5);
        mat4x4 scale = mat4scale(1,1,1);
        
        mat4x4 mv = mat4mul4(&scale, &rotation);
        mv = mat4mul4(&translation, &mv);
        */
        mat4x4 mv = mat4transform(1,1,1,0,0,-5,t,0,0);
        set_modelview(&mv);

        render();


        SR_Present(window);
    
        printf("frame in: %fms\n",delta * 1000);
    }

    SR_DestroyWindow(window);

    return 0;
}

