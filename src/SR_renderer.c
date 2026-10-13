#include "SR.h"
#include "SR_matrix.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

#define SR_PI 3.14159

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define CLAMP(v, a, b) (MAX(MIN(v,b),a))


#define OPENGL_COORDS

#ifdef OPENGL_COORDS // -1 -> +1
#define COORD_NORMALISE(f) ((f+1)/2.0f)
#else // 0 -> 1
#define COORD_NORMALISE(f) (f)
#endif

#define NORMALISE_COORD_X(f) ((int)roundf(COORD_NORMALISE(f) * scr_w))
#define NORMALISE_COORD_Y(f) ((int)roundf(COORD_NORMALISE(f) * scr_h))


typedef struct{
    col4 *px;
    SR_COLOURFORMAT fmt;
} SR_DestBuf;



#define SR_MAX_PIPELINE_PROPS 16
typedef struct {
    int mStride; //size of the vertex in bytes

    int mPosStart; //start of the position (3 bytes)

    SR_COLOURFORMAT mColFmt; //colour format
    int mColStart; //start of the colour format(specifeid by mColFmt)
} SR_VertexLayout;

typedef struct {
    float4 pos; //position
    fcol4 col; //colour 
    float2 uv; //UV coords
} SR_Point;

typedef struct SR_Pipeline{
    //pipeline funcs
    float4 (*mVertexFunc)(mat4x4* modelview, mat4x4* persp, float x, float y, float z);
    fcol4 (*mPixelFunc)(SR_Point* pixel);
    void (*mRasterizeFunc)(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3);

    //state objects
    SR_DestBuf mDest;
    SR_VertexLayout mVertexLayout;
    

    mat4x4 mPersp;
    mat4x4 mModelView;
    int mCullmode;
    int mDepthTest;

    int mScrWidth;
    int mScrHeight;

    //misc objects
    float *mDepthBuffer;
    
} SR_Pipeline;

//internal funcs

void _SR_AllocDepthBuf(SR_Pipeline* pipeline){
    if(pipeline->mDepthBuffer){
        free(pipeline->mDepthBuffer);
    }
    pipeline->mDepthBuffer = (float*)malloc(pipeline->mScrWidth * pipeline->mScrHeight * sizeof(float));
}
void _SR_FreeDepthBuf(SR_Pipeline* pipeline){
    if(pipeline->mDepthBuffer){
        free(pipeline->mDepthBuffer);
    }
    pipeline->mDepthBuffer = NULL;
}

//State Management

float4 SR_DefaultVertexFunc(mat4x4* modelview, mat4x4* persp, float x, float y, float z);
fcol4 SR_DefaultPixelFunc(SR_Point* p);
void SR_RasterizeTris(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3);
void SR_RasterizeLines(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3);


SR_Pipeline* SR_CreatePipeline(void* initalTarget, int w, int h, SR_COLOURFORMAT format){

    SR_Pipeline* pipeline = malloc(sizeof(SR_Pipeline));

    //bind initial data
    pipeline->mDest.px = (col4*)initalTarget;
    pipeline->mDest.fmt = format;
    pipeline->mScrWidth = w;
    pipeline->mScrHeight = h;

    //no depth buffer by default
    pipeline->mDepthBuffer = NULL;

    //default matrices
    pipeline->mModelView = mat4identity();
    pipeline->mPersp = mat4identity();

    pipeline->mCullmode = SR_CULL_NONE;
    pipeline->mDepthTest = 0;

    pipeline->mVertexFunc = SR_DefaultVertexFunc;
    pipeline->mPixelFunc = SR_DefaultPixelFunc;
    pipeline->mRasterizeFunc = SR_RasterizeTris;

    return pipeline;
}

void SR_DestroyPipeline(SR_Pipeline* pipeline){
    if(pipeline->mDepthTest){
        _SR_FreeDepthBuf(pipeline);
    }

    free(pipeline);
}

//Binding Functions


void SR_ToggleDepthTest(SR_Pipeline* pipeline, int enabled){
    pipeline->mDepthTest = !!(enabled);

    if(enabled){
        _SR_AllocDepthBuf(pipeline);
    }
    else{
        _SR_FreeDepthBuf(pipeline);
    }
    
}

SR_ERROR SR_UpdateRenderTarget(SR_Pipeline* pipeline, void* target, int w, int h, SR_COLOURFORMAT format){
    pipeline->mDest.px = (col4*)target;
    pipeline->mDest.fmt = format;
    pipeline->mScrWidth = w;
    pipeline->mScrHeight = h;

    if(pipeline->mDepthTest){
        _SR_AllocDepthBuf(pipeline);
    }
    return SR_ERR_NONE;
}

SR_ERROR SR_BindLayout(SR_Pipeline* pipeline, SR_LAYOUT_PROP* props, int count ){

    // TODO: Validate that we haved passed a broken layout prop to the function (ie more than one position etc)

    int b_offset = 0;
    //copy props over
    for (int i = 0; i < count; ++i)
    {
        switch (props[i])
        {
        case SR_LAYOUT_POS_XYZ_F:
            pipeline->mVertexLayout.mPosStart = b_offset;
            b_offset += 3;
            break;
        case SR_LAYOUT_COLOUR_RGB_F:
            pipeline->mVertexLayout.mColFmt = SR_COLOUR_RGB24;
            pipeline->mVertexLayout.mColStart = b_offset;
            b_offset += 3;
            break;  
        default:
            break;
        }
    }
    pipeline->mVertexLayout.mStride = b_offset;
    
    return SR_ERR_NONE;
}

void SR_BindModelview(SR_Pipeline* pipeline, mat4x4* matrix){
    pipeline->mModelView = *matrix;
}


void SR_PerspFOV(SR_Pipeline* pipeline, float near, float far, float aspect, float FOV){
    float e = 1.0f / tanf((FOV * (SR_PI / 180.0f))/2.0f); 

    printf("e= %f\n",e);

    pipeline->mPersp = (mat4x4){
        e/aspect,               0,                      0,      0,
        0,                      e,                      0,      0,
        0,                      0,                      (far+near)/(near-far),          (2*far*near)/(near-far),
        0,                      0,                      -1,                             0,
    };
}

void SR_Persp(SR_Pipeline* pipeline, float near, float far, float left, float right, float top, float bottom){

    pipeline->mPersp = (mat4x4){
        (2*near)/(right-left),  0,                      (right+left)/(right-left),      0,
        0,                      (2*near)/(top-bottom),  (top+bottom)/(top-bottom),      0,
        0,                      0,                      (far+near)/(near-far),          (2*far*near)/(near-far),
        0,                      0,                      -1,                             0,
    };
}

//Drawing Functions


col4 SR_TransformToPixelColour(SR_COLOURFORMAT fmt, fcol4 v){

    switch (fmt)
    {
    case SR_COLOUR_RGBA32:
        return (col4) {CLAMP(v.r,0,1) * 255,CLAMP(v.g,0,1) * 255,  CLAMP(v.b,0,1) * 255, CLAMP(v.a,0,1) * 255};
    
    case SR_COLOUR_BGRA32:
        return (col4) {CLAMP(v.b,0,1) * 255,CLAMP(v.g,0,1) * 255,  CLAMP(v.r,0,1) * 255, CLAMP(v.a,0,1) * 255};
    
    default:
        return (col4){0,0,0,255}; //if err, return black;
    }

}


void SR_Clear(SR_Pipeline* pipeline, fcol4 c){

    col4 pix = SR_TransformToPixelColour(pipeline->mDest.fmt, c);
    unsigned long long count = pipeline->mScrWidth * pipeline->mScrHeight;
    for (unsigned long long i = 0; i < count; i++)
    {
        pipeline->mDest.px[i] = pix;
    }
    
}

void SR_ClearDepthBuffer(SR_Pipeline* pipeline){
    //dont clear if depth testing not enabled
    if(pipeline->mDepthTest == 0) return;

    unsigned long long count = pipeline->mScrWidth * pipeline->mScrHeight;
    for (size_t i = 0; i < count; i++)
    {
        pipeline->mDepthBuffer[i] = 1000000.0f;
    }
}



float4 SR_DefaultVertexFunc(mat4x4* modelview, mat4x4* persp, float x, float y, float z){
    float4 p = mat4mul(modelview, (float4){x,y,z,1.0f});
    p = mat4mul(persp,p);
    return p;
}

fcol4 SR_DefaultPixelFunc(SR_Point* p){
    return p->col;
}


void SR_DrawPixel(SR_Pipeline* pipeline, int x, int y, fcol4 col){
    if(!pipeline->mDest.px){
        return;
    }
    if (x >= 0 && x < pipeline->mScrWidth && y >= 0 && y < pipeline->mScrHeight){

        //transform and copy
        pipeline->mDest.px[x + (y * pipeline->mScrWidth)] = SR_TransformToPixelColour(pipeline->mDest.fmt, col);
    }
}

void SR_DrawPoint(SR_Pipeline* pipeline, int x, int y, SR_Point* p){
    if(!pipeline->mDest.px){
        return;
    }
    if (x >= 0 && x < pipeline->mScrWidth && y >= 0 && y < pipeline->mScrHeight){

        
        //transform and copy
        pipeline->mDest.px[x + (y * pipeline->mScrWidth)] = SR_TransformToPixelColour(pipeline->mDest.fmt, pipeline->mPixelFunc(p));


    }
}


void SR_RasterizeTris(
    SR_Pipeline* pipeline,
    SR_Point* p1, 
    SR_Point* p2,  
    SR_Point* p3){

    int min_x = (MIN(MIN(p1->pos.x, p2->pos.x), p3->pos.x));
    int max_x = (MAX(MAX(p1->pos.x, p2->pos.x), p3->pos.x));
    int min_y = (MIN(MIN(p1->pos.y, p2->pos.y), p3->pos.y));
    int max_y = (MAX(MAX(p1->pos.y, p2->pos.y), p3->pos.y));

    float f = (p1->pos.x *(p2->pos.y - p3->pos.y)) + (p2->pos.x *(p3->pos.y - p1->pos.y)) + (p3->pos.x *(p1->pos.y - p2->pos.y));

    f = 1/f; //invert

    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;

    m00 = (p2->pos.x*p3->pos.y) - (p3->pos.x*p2->pos.y);
    m10 = (p3->pos.x*p1->pos.y) - (p1->pos.x*p3->pos.y);
    m20 = (p1->pos.x*p2->pos.y) - (p2->pos.x*p1->pos.y);

    m01 = p2->pos.y - p3->pos.y;
    m11 = p3->pos.y - p1->pos.y;
    m21 = p1->pos.y - p2->pos.y;
    
    m02 = p3->pos.x - p2->pos.x;
    m12 = p1->pos.x - p3->pos.x;
    m22 = p2->pos.x - p1->pos.x;

    //(B - A) x (C - A)
    //calculate the z property of the cross product to get the winding of the triangle
    //x and y cancel out to 0 when z is 0 (ie a flat triangle)
    

    if(pipeline->mCullmode != 0){ //cullmode of zero means dont cull
        int winding = (-(p2->pos.y - p1->pos.y) * (p3->pos.x - p1->pos.x)) + ((p2->pos.x - p1->pos.x) * (p3->pos.y - p1->pos.y));

        printf("Winding = %d\n",winding);
        //do some bitwise trickery to determine wether to cull or not
        if((winding ^ pipeline->mCullmode) >= 0){
            return;
        }
    }





    for (int j = min_y; j < max_y; j++)
    {
        for (int i = min_x; i < max_x; i++)
        {
            float fi = i + 0.5f;
            float fj = j + 0.5f;
            
            float bx =  ((m00 * 1) + (m01 * fi) + (m02 * fj)) * f;
            float by =  ((m10 * 1) + (m11 * fi) + (m12 * fj)) * f;
            float bz =  ((m20 * 1) + (m21 * fi) + (m22 * fj)) * f;

            //in triangle
            if(bx > -0 && by > 0 && bz > 0){

                SR_Point p;
                // first calculate the depth and throw this pixel away if its not going to be visible
                // dont calculate everything first if we dont have to
                p.pos.w = (p1->pos.w * bx) + (p2->pos.w * by) + (p3->pos.w * bz);
                
                //only check for depth if there is a depth buffer bound
                if(pipeline->mDepthBuffer){
                    if(p.pos.w > pipeline->mDepthBuffer[i + j*pipeline->mScrWidth]){
                        continue;
                    }
                    pipeline->mDepthBuffer[i + j*pipeline->mScrWidth] = p.pos.w;
                }

                //this pixel is going to be rendered, calculate eveything else
                p.pos.x = (p1->pos.x * bx) + (p2->pos.x * by) + (p3->pos.x * bz);
                p.pos.y = (p1->pos.y * bx) + (p2->pos.y * by) + (p3->pos.y * bz);
                p.pos.z = (p1->pos.z * bx) + (p2->pos.z * by) + (p3->pos.z * bz);


                p.col.r = (p1->col.r * bx) + (p2->col.r * by) + (p3->col.r * bz);
                p.col.g = (p1->col.g * bx) + (p2->col.g * by) + (p3->col.g * bz);
                p.col.b = (p1->col.b * bx) + (p2->col.b * by) + (p3->col.b * bz);
                p.col.a = 1; //no transparency support yet so alpha is hardcoded to 1


                SR_DrawPoint(pipeline, i,j, &p);
            }
        }
    }
    
}


static void line_low(SR_Pipeline* pipeline, int x1, int y1, int x2, int y2){
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

        SR_DrawPixel(pipeline,x, y,(fcol4){0,0,0,1});
        if(D > 0){
            y += yi;
            D = D + (2 * (dy - dx));
        }
        else{
            D = D + 2*dy;
        }
    }

}

static void line_high(SR_Pipeline* pipeline, int x1, int y1, int x2, int y2){
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

        SR_DrawPixel(pipeline,x, y,(fcol4){0,0,0,1});
        if(D > 0){
            x += xi;
            D = D + (2 * (dx - dy));
        }
        else{
            D = D + 2*dx;
        }
    }

}

static void bresenham(SR_Pipeline* pipeline, int x1, int y1, int x2, int y2){

    //printf("[%d, %d] -> [%d, %d]\n",x1,y1,x2,y2);
    if(abs(y2 - y1) < abs(x2 - x1)){
        if(x1 > x2){
            line_low(pipeline, x2,y2,x1,y1);
        }
        else{
            line_low(pipeline, x1,y1,x2,y2);
        }
    }
    else{
        if(y1 > y2){
            line_high(pipeline, x2,y2,x1,y1);
        }
        else{
            line_high(pipeline, x1,y1,x2,y2);
        }
    }
}

void SR_RasterizeLines(SR_Pipeline* pipeline,
    SR_Point* p1, 
    SR_Point* p2,  
    SR_Point* p3){
    bresenham(pipeline, p1->pos.x,p1->pos.y,p2->pos.x,p2->pos.y);
    bresenham(pipeline, p2->pos.x,p2->pos.y,p3->pos.x,p3->pos.y);
    bresenham(pipeline, p1->pos.x,p1->pos.y,p3->pos.x,p3->pos.y);
}


//#define LOG_DRAW

void SR_DrawIndexed(SR_Pipeline* pipeline, float* vertices, int* indices, unsigned long long count){
#ifdef LOG_DRAW
    puts("----BEGINRENDER----");
#endif
    int c = 0;

    //extract these before the loop so we dont have to access a pointer in the tight loop
    mat4x4* persp = &pipeline->mPersp;
    mat4x4* modelview = &pipeline->mModelView;

    SR_VertexLayout layout = pipeline->mVertexLayout;


    count = count * 3;
    for (unsigned long long i = 0; i < count; i += 3)
    {
        
        SR_Point p[3] = {0};

        int ind[3];
        ind[0] = (indices[i+0] * layout.mStride);
        ind[1] = (indices[i+1] * layout.mStride);
        ind[2] = (indices[i+2] * layout.mStride);
#ifdef LOG_DRAW
        puts("--BEGINTRI--");
#endif
        for (int v = 0; v < 3; v++)
        {
            float* f = vertices + ind[v];

            switch (layout.mColFmt)
            {
            case SR_COLOUR_EMPTY:
                //no colour
                p[v].col = (fcol4){1,1,1,1};
                break;
            case SR_COLOUR_RGB24:
                memcpy(&p[v].col, f + layout.mColStart, 3 * sizeof(float));
                p[v].col.a = 1;
                break;
            case SR_COLOUR_RGBA32:
                memcpy(&p[v].col, f + layout.mColStart, 4 * sizeof(float));
                break;
            default:
                break;
            }
            
            p[v].pos = pipeline->mVertexFunc(modelview, persp, f[layout.mPosStart],f[layout.mPosStart + 1],f[layout.mPosStart + 2]);

            int ox = COORD_NORMALISE(p[v].pos.x / p[v].pos.z) * pipeline->mScrWidth;
            int oy = COORD_NORMALISE(p[v].pos.y / p[v].pos.z) * pipeline->mScrHeight;
#ifdef LOG_DRAW
            printf("[%d] -> (%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f,%.1f) -> (%d,%d)\n",ind[v], f[layout.mPosStart],f[layout.mPosStart + 1],f[layout.mPosStart + 1],1.0f,p[v].pos.x,p[v].pos.y,p[v].pos.z,p[v].pos.w, ox,oy);
#endif
            p[v].pos.x = ox;
            p[v].pos.y = oy;
        }


        //SR_RasterizeTris(pipeline, &p[0], &p[1], &p[2]);
        //SR_RasterizeLines(pipeline, &p[0], &p[1], &p[2]);
        pipeline->mRasterizeFunc(pipeline, &p[0], &p[1], &p[2]);
#ifdef LOG_DRAW
        puts("--ENDTRI--");
#endif
        c++;
    }
#ifdef LOG_DRAW
     puts("----ENDRENDER----");
    //printf("rendered %d tris\n",c);
#endif
    
}