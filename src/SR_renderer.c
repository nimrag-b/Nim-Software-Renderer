#include "SR.h"
#include "SR_matrix.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <float.h>

//#define SR_AFFINE_MAPPING

//#define SR_DONT_VALIDATE
// 
// this option will strip all validation, meaning that functions will never check to make sure that its accessing optional data.
// This can potentially cause code to run faster
// ONLY ENABLE IF: 
//      a texture is always bound
//      
//      every model has a texcoord, normal, and colour data


#define SR_PI 3.14159

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define CLAMP(v, a, b) (((v) < (a)) ? (a) : ((v) > (b)) ? (b) : (v))


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

    int mTexcoordStart;

    int mNormalStart;

    SR_COLOURFORMAT mColFmt; //colour format
    int mColStart; //start of the colour format(specifeid by mColFmt)


    char mHasColour;
    char mHasTexCoord;
    char mHasNormal;
} SR_VertexLayout;



typedef struct SR_Pipeline{
    //pipeline funcs
    SR_VertexFunc mVertexFunc;
    SR_PixelFunc mPixelFunc;
    void (*mRasterizeFunc)(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3, float4* s1, float4* s2, float4* s3);

    //state objects
    SR_DestBuf mDest;
    SR_VertexLayout mVertexLayout;
    SR_Texture* mTextures[SR_MAX_TEXTURES];
    SR_RenderProps mRenderProps;

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

void _SR_AllocDepthBuf(SR_Pipeline* pipeline, int w, int h){
    if(pipeline->mDepthBuffer){
        free(pipeline->mDepthBuffer);
    }
    pipeline->mDepthBuffer = (float*)malloc(w * h * sizeof(float));
}
void _SR_FreeDepthBuf(SR_Pipeline* pipeline){
    if(pipeline->mDepthBuffer){
        free(pipeline->mDepthBuffer);
    }
    pipeline->mDepthBuffer = NULL;
}

//State Management

float4 SR_DefaultVertexFunc(mat4x4* modelview, mat4x4* persp, float x, float y, float z);
col4 SR_DefaultPixelFunc(SR_Point* p, SR_Texture** textures);
void SR_RasterizeTris(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3);
void SR_RasterizeLines(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3);

int SR_ColFormatLength(SR_COLOURFORMAT fmt);


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

    memset(pipeline->mTextures,0,sizeof(pipeline->mTextures));

    pipeline->mCullmode = SR_CULL_NONE;
    pipeline->mDepthTest = 0;

    pipeline->mVertexFunc = SR_DefaultVertexFunc;
    pipeline->mPixelFunc = SR_DefaultPixelFunc;
    pipeline->mRasterizeFunc = SR_RasterizeTris;

    pipeline->mRenderProps.lightDir = (float3){ 0.5f,0.5f,0 };
    pipeline->mRenderProps.viewPos = (float3){ 0,0,0 };

    return pipeline;
}

void SR_DestroyPipeline(SR_Pipeline* pipeline){
    if(pipeline->mDepthTest){
        _SR_FreeDepthBuf(pipeline);
    }

    free(pipeline);
}


SR_Texture* SR_CreateTexture(void* data, int w, int h, SR_COLOURFORMAT fmt){
    SR_Texture* texture = malloc(sizeof(SR_Texture));
    if (texture == NULL) return NULL;
    texture->w = w;
    texture->h = h;

    int fmtLength = SR_ColFormatLength(fmt);

    texture->pixels = malloc(texture->w * texture->h * sizeof(col4));
    if (texture->pixels == NULL) {
        free(texture);
        return NULL;
    }
    switch (fmt)
    {
        case SR_COLOUR_RGBA32:
            memcpy(texture->pixels, data, texture->w * texture->h * sizeof(col4));
            break;
        case SR_COLOUR_RGB24:
            for (size_t j = 0; j < h; j++)
            {
                for (size_t i = 0; i < w; i++)
                {
                    char* ptr = (char*)data + ((i + (j * w)) * 3);
                    col4 c;
                    c.r = ptr[0];
                    c.g = ptr[1];
                    c.b = ptr[2];
                    c.a = 255;
                    texture->pixels[i + (j * w)] = c;
                }
            }
            break;
        case SR_COLOUR_BGRA32:
            for (size_t j = 0; j < h; j++)
            {
                for (size_t i = 0; i < w; i++)
                {
                    char* ptr = (char*)data + ((i + (j * w)) * 4);
                    col4 c;
                    c.b = ptr[0];
                    c.g = ptr[1];
                    c.r = ptr[2];
                    c.a = ptr[3];
                    texture->pixels[i + (j * w)] = c;
                }
            }
            break;
    default:
        free(texture->pixels);
        free(texture);
        return NULL;
        break;
    }
    
    return texture;
}

void SR_DestroyTexture(SR_Texture* texture){
    free(texture->pixels);
    free(texture);
}

//Binding Functions


void SR_ToggleDepthTest(SR_Pipeline* pipeline, int enabled){
    pipeline->mDepthTest = !!(enabled);

    if(enabled){
        _SR_AllocDepthBuf(pipeline, pipeline->mScrWidth, pipeline->mScrHeight);
    }
    else{
        _SR_FreeDepthBuf(pipeline);
    }
    
}

void SR_SetCullMode(SR_Pipeline* pipeline, enum SR_CULLMODE mode){
    pipeline->mCullmode = mode;
}

SR_ERROR SR_UpdateRenderTarget(SR_Pipeline* pipeline, void* target, int w, int h, SR_COLOURFORMAT format){

    if (pipeline->mDepthTest) {
        //dont recreate depth buffer if the screen size is the same
        if (w != pipeline->mScrWidth || h != pipeline->mScrHeight) {
            _SR_AllocDepthBuf(pipeline,w,h);
        }
    }

    pipeline->mDest.px = (col4*)target;
    pipeline->mDest.fmt = format;
    pipeline->mScrWidth = w;
    pipeline->mScrHeight = h;


    return SR_ERR_NONE;
}

SR_UpdateRenderProps(SR_Pipeline* pipeline, SR_RenderProps* props) {
    memcpy(&pipeline->mRenderProps, props, sizeof(SR_RenderProps));
}

void SR_BindVertexFunc(SR_Pipeline* pipeline, SR_VertexFunc func) {
    if (func == NULL) {
        pipeline->mVertexFunc = SR_DefaultVertexFunc;
    }
    else {
        pipeline->mVertexFunc = func;
    }
}
void SR_BindPixelFunc(SR_Pipeline* pipeline, SR_PixelFunc func) {
    if (func == NULL) {
        pipeline->mPixelFunc = SR_DefaultPixelFunc;
    }
    else {
        pipeline->mPixelFunc = func;
    }
}

SR_ERROR SR_BindLayout(SR_Pipeline* pipeline, SR_LAYOUT_PROP* props, int count ){

    // TODO: Validate that we haved passed a broken layout prop to the function (ie more than one position etc)

    memset(&pipeline->mVertexLayout, 0, sizeof(pipeline->mVertexLayout));

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
        case SR_LAYOUT_NORMAL_F:
            pipeline->mVertexLayout.mHasNormal = 1;
            pipeline->mVertexLayout.mNormalStart = b_offset;
            b_offset += 3;
            break;
        case SR_LAYOUT_COLOUR_RGB_F:
            pipeline->mVertexLayout.mHasColour = 1;
            pipeline->mVertexLayout.mColFmt = SR_COLOUR_RGB24;
            pipeline->mVertexLayout.mColStart = b_offset;
            b_offset += 3;
            break;  
        case SR_LAYOUT_TEXCOORD_UV_F:
            pipeline->mVertexLayout.mHasTexCoord = 1;
            pipeline->mVertexLayout.mTexcoordStart = b_offset;
            b_offset += 2;
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

SR_ERROR SR_BindTexture(SR_Pipeline* pipeline, SR_Texture* texture, int slot){
    if(slot >= SR_MAX_TEXTURES){
        return SR_ERR_TOO_LONG;
    }
    pipeline->mTextures[slot] = texture;
    return SR_ERR_NONE;
}


void SR_PerspFOV(SR_Pipeline* pipeline, float near, float far, float aspect, float FOV){
    float e = 1.0f / tanf((FOV * (SR_PI / 180.0f))/2.0f); 
    
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

col4 SR_RGBAToDestPixelColour(SR_COLOURFORMAT fmt, col4 v) {
    switch (fmt)
    {
    case SR_COLOUR_RGBA32:
        return v;

    case SR_COLOUR_RGB24:
        return (col4) { v.r, v.g, v.b, 255 };

    case SR_COLOUR_BGRA32:
        return (col4) { v.b, v.g, v.r, v.a };

    default:
#ifdef SR_DONT_VALIDATE
        return v;
#else
        printf("err format %d\n", fmt);
        return (col4) { 0, 0, 0, 255 }; //if err, return black;
#endif // SR_DONT_VALIDATE


    }
}

col4 SR_TransformToPixelColour(SR_COLOURFORMAT fmt, fcol4 v){

    switch (fmt)
    {
    case SR_COLOUR_RGBA32:
        return (col4) {CLAMP(v.r,0,1) * 255,CLAMP(v.g,0,1) * 255,  CLAMP(v.b,0,1) * 255, CLAMP(v.a,0,1) * 255};

    case SR_COLOUR_RGB24:
        return (col4) { CLAMP(v.r, 0, 1) * 255, CLAMP(v.g, 0, 1) * 255, CLAMP(v.b, 0, 1) * 255, 255 };
    
    case SR_COLOUR_BGRA32:
        return (col4) {CLAMP(v.b,0,1) * 255,CLAMP(v.g,0,1) * 255,  CLAMP(v.r,0,1) * 255, CLAMP(v.a,0,1) * 255};
    
    default:
#ifdef SR_DONT_VALIDATE
        return (col4) { 0, 0, 0, 255 };
#else
        printf("err format %d\n", fmt);
        return (col4) { 0, 0, 0, 255 }; //if err, return black;
#endif // SR_DONT_VALIDATE

    }

}

fcol4 SR_TransformToFloatColour(SR_COLOURFORMAT fmt, col4 v){

    switch (fmt)
    {
    case SR_COLOUR_RGBA32:
        return (fcol4) {v.r / 255.0f,v.g / 255.0f,v.b / 255.0f,v.a / 255.0f};

    case SR_COLOUR_RGB24:
        return (fcol4) { v.r / 255.0f, v.g / 255.0f, v.b / 255.0f, 1 };
    
    case SR_COLOUR_BGRA32:
        return (fcol4) {v.b / 255.0f,v.g / 255.0f,v.r / 255.0f,v.a / 255.0f};
    default:
#ifdef SR_DONT_VALIDATE
        return (fcol4) { 0, 0, 0, 1 };
#else
        printf("err format %d\n", fmt);
        return (fcol4) { 0, 0, 0, 1 }; //if err, return black;
#endif // SR_DONT_VALIDATE


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

    memset(pipeline->mDepthBuffer,127,count*sizeof(float));
    //printf("clrdpth = %f\n", pipeline->mDepthBuffer[0]);
}

col4 colcomb(col4 a, col4 b){
    //use bitshifting to efficiently move into range
    return (col4){(a.r * b.r) >> 8, (a.g * b.g) >> 8, (a.b * b.b) >> 8, (a.a * b.a) >> 8};
}

col4 coladd(col4 a, col4 b) {
    return (col4) { CLAMP(a.r + b.r,0,255), CLAMP(a.g + b.g, 0, 255), CLAMP(a.b + b.b, 0, 255), CLAMP(a.a + b.a, 0, 255)};
}


float4 SR_DefaultVertexFunc(mat4x4* modelview, mat4x4* persp, SR_Point* pixel){
    float4 p = mat4mul(modelview, (float4){pixel->pos.x, pixel->pos.y, pixel->pos.z,1.0f});
    pixel->normal = mat4mul3(modelview, pixel->normal);
    p = mat4mul(persp,p);
    return p;
}

col4 SR_Sample(SR_Texture* texture, float x, float y);


col4 col3to4(col3 col) {
    return (col4) { col.r, col.g, col.b, 255 };
}

float float3length(float3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float3 float3normalize(float3 v) {
    float l = float3length(v);
    return (float3) { v.x / l, v.y / l, v.z / l };
}

float float3dot(float3 a, float3 b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

float3 float3add(float3 a, float3 b) {
    return (float3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

#define COL4CONV(f4) (col4){CLAMP(f4.r,0,1)*255,CLAMP(f4.g,0,1)*255,CLAMP(f4.b,0,1)*255,CLAMP(f4.a,0,1)*255}

col4 SR_DefaultPixelFunc(SR_RenderProps* props, SR_Point* p, SR_Texture** textures){


    float3 lightDir = float3normalize(props->lightDir);

    float diff = CLAMP(float3dot(p->normal, lightDir), 0, 1.0);

    col4 color = SR_Sample(textures[0], p->uv.x, p->uv.y);
    float factor = 0.2f;
    // ambient
    col4 ambient = (col4){ factor * color.r, factor * color.g, factor * color.b,255 };
    // diffuse

    col4 diffuse = (col4){ diff * color.r, diff * color.g,diff * color.b,255 };


    return coladd(ambient, diffuse);
}




int SR_ColFormatLength(SR_COLOURFORMAT fmt){
    switch (fmt)
    {
    case SR_COLOUR_BGRA32:
    case SR_COLOUR_RGBA32:
        return 4;
    
    case SR_COLOUR_RGB24:
        return 3;
       
    
    case SR_COLOUR_EMPTY:
    default:
        return 0;
    }
}

struct char3 {
    char a, b, c;
};
struct char4 {
    char a, b, c, d;
};

col4 SR_Sample(SR_Texture* texture, float x, float y){
#ifndef SR_DONT_VALIDATE
    if(!texture) return (col4){255,255,255,255};
#endif
    int i = CLAMP(x,0,1) * texture->w;
    int j = CLAMP(y,0,1) * texture->h;

    return texture->pixels[i + (j * texture->w)];
}


void SR_DrawPixelUnchecked(SR_Pipeline* pipeline, int x, int y, col4 col){
    //transform and copy
    pipeline->mDest.px[x + (y * pipeline->mScrWidth)] = SR_RGBAToDestPixelColour(pipeline->mDest.fmt, col);
}

void SR_DrawPixel(SR_Pipeline* pipeline, int x, int y, col4 col){


    if(!pipeline->mDest.px){
        return;
    }

    if (x >= 0 && x < pipeline->mScrWidth && y >= 0 && y < pipeline->mScrHeight){

        SR_DrawPixelUnchecked(pipeline, x, y, col);
    }
}



void SR_RasterizeTris(
    SR_Pipeline* pipeline,
    SR_Point* p1, SR_Point* p2, SR_Point* p3,
    float4* s1, float4* s2, float4* s3){

    int min_x = CLAMP((MIN(MIN(s1->x, s2->x), s3->x)), 0, pipeline->mScrWidth);
    int max_x = CLAMP((MAX(MAX(s1->x, s2->x), s3->x)) + 1, 0, pipeline->mScrWidth);
    int min_y = CLAMP((MIN(MIN(s1->y, s2->y), s3->y)), 0, pipeline->mScrHeight);
    int max_y = CLAMP((MAX(MAX(s1->y, s2->y), s3->y)) + 1, 0, pipeline->mScrHeight);

    //point offscreen
    if(max_x == 0 || max_y == 0){
        return;
    }

    float f = (s1->x *(s2->y - s3->y)) + (s2->x *(s3->y - s1->y)) + (s3->x *(s1->y - s2->y));

    f = 1/f; //invert

    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;

    m00 = (s2->x*s3->y) - (s3->x*s2->y);
    m10 = (s3->x*s1->y) - (s1->x*s3->y);
    m20 = (s1->x*s2->y) - (s2->x*s1->y);

    m01 = s2->y - s3->y;
    m11 = s3->y - s1->y;
    m21 = s1->y - s2->y;
    
    m02 = s3->x - s2->x;
    m12 = s1->x - s3->x;
    m22 = s2->x - s1->x;

    //(B - A) x (C - A)
    //calculate the z property of the cross product to get the winding of the triangle
    //x and y cancel out to 0 when z is 0 (ie a flat triangle)
    

    if(pipeline->mCullmode != 0){ //cullmode of zero means dont cull
        int winding = (-(s2->y - s1->y) * (s3->x - s1->x)) + ((s2->x - s1->x) * (s3->y - s1->y));

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
            float fi = i;// +0.5f;
            float fj = j;// +0.5f;
            
            float bx =  ((m00 * 1) + (m01 * fi) + (m02 * fj)) * f;
            float by =  ((m10 * 1) + (m11 * fi) + (m12 * fj)) * f;
            float bz =  ((m20 * 1) + (m21 * fi) + (m22 * fj)) * f;

            //in triangle
            if(bx > -0 && by > 0 && bz > 0){

                SR_Point p;
                // first calculate the depth and throw this pixel away if its not going to be visible
                // dont calculate everything first if we dont have to
                
                float d = (s1->w * bx) + (s2->w * by) + (s3->w * bz);
                //only check for depth if there is a depth buffer bound
                if(pipeline->mDepthBuffer){
                    //printf("depth: %f, buffer: %f\n", p.pos.w, pipeline->mDepthBuffer[i + j * pipeline->mScrWidth]);
                    if(d > pipeline->mDepthBuffer[i + j*pipeline->mScrWidth]){
                        continue;
                    }
                    pipeline->mDepthBuffer[i + j*pipeline->mScrWidth] = d;
                }

                //this pixel is going to be rendered, calculate eveything else
                p.pos.x = (p1->pos.x * bx) + (p2->pos.x * by) + (p3->pos.x * bz);
                p.pos.y = (p1->pos.y * bx) + (p2->pos.y * by) + (p3->pos.y * bz);
                p.pos.z = (p1->pos.z * bx) + (p2->pos.z * by) + (p3->pos.z * bz);
                p.pos.w = d; //for now, store the depth in w in case we need it

                p.col.r = (p1->col.r * bx) + (p2->col.r * by) + (p3->col.r * bz);
                p.col.g = (p1->col.g * bx) + (p2->col.g * by) + (p3->col.g * bz);
                p.col.b = (p1->col.b * bx) + (p2->col.b * by) + (p3->col.b * bz);
                p.col.a = 255; //no transparency support yet so alpha is hardcoded to 1

#ifdef SR_AFFINE_MAPPING
                p.uv.x = ((p1->uv.x) * bx) + ((p2->uv.x) * by) + ((p3->uv.x) * bz);
                p.uv.y = ((p1->uv.y) * bx) + ((p2->uv.y) * by) + ((p3->uv.y) * bz);
#else
                float ui = ((p1->uv.x / s1->w) * bx) + ((p2->uv.x / s2->w) * by) + ((p3->uv.x / s3->w) * bz);
                float vi = ((p1->uv.y / s1->w) * bx) + ((p2->uv.y / s2->w) * by) + ((p3->uv.y / s3->w) * bz);
                float zi = ((1 / s1->w) * bx) + ((1 / s2->w) * by) + ((1 / s3->w) * bz);

                p.uv.x = ui / zi;
                p.uv.y = vi / zi;
#endif
                //scale UV by depth for accurate shading
                //p.uv.x /= -p.pos.w;
                //p.uv.y /= -p.pos.w;

                p.normal.x = (p1->normal.x * bx) + (p2->normal.x * by) + (p3->normal.x * bz);
                p.normal.y = (p1->normal.y * bx) + (p2->normal.y * by) + (p3->normal.y * bz);
                p.normal.z = (p1->normal.z * bx) + (p2->normal.z * by) + (p3->normal.z * bz);

                //pixel should be on screen by this point
                SR_DrawPixelUnchecked(pipeline, i,j, pipeline->mPixelFunc(&pipeline->mRenderProps, &p, pipeline->mTextures));
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

        SR_DrawPixel(pipeline,x, y,(col4){0,0,0,255});
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

        SR_DrawPixel(pipeline,x, y,(col4){0,0,0,255});
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

    //assert that a destination does exit
    if(!pipeline->mDest.px){
        puts("WARNING: Destination not set in SR_pipeline");
        return;
    }

    int c = 0;

    //extract these before the loop so we dont have to access a pointer in the tight loop
    mat4x4* persp = &pipeline->mPersp;
    mat4x4* modelview = &pipeline->mModelView;

    SR_VertexLayout layout = pipeline->mVertexLayout;


    count = count * 3;
    for (unsigned long long i = 0; i < count; i += 3)
    {
        
        SR_Point p[3] = {0};
        float4 scr_pos[3] = {0};

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
                p[v].col = (col4){255,255,255,255};
                break;
            case SR_COLOUR_RGB24:
                p[v].col.r = f[layout.mColStart] * 255;
                p[v].col.g = f[layout.mColStart + 1] * 255;
                p[v].col.b = f[layout.mColStart + 2] * 255;
                p[v].col.a = 255;
                break;
            case SR_COLOUR_RGBA32:
                p[v].col.r = f[layout.mColStart] * 255;
                p[v].col.g = f[layout.mColStart + 1] * 255;
                p[v].col.b = f[layout.mColStart + 2] * 255;
                p[v].col.a = f[layout.mColStart + 3] * 255;
                break;
            default:
                break;
            }

            p[v].pos.x = f[layout.mPosStart];
            p[v].pos.y = f[layout.mPosStart + 1];
            p[v].pos.z = f[layout.mPosStart + 2];


            if (layout.mHasTexCoord) {
                p[v].uv.x = f[layout.mTexcoordStart];
                p[v].uv.y = f[layout.mTexcoordStart + 1];
            }

            if (layout.mHasNormal) {
                p[v].normal.x = f[layout.mNormalStart];
                p[v].normal.y = f[layout.mNormalStart + 1];
                p[v].normal.z = f[layout.mNormalStart + 2];
            }

            scr_pos[v] = pipeline->mVertexFunc(modelview, persp, &p[v]);

            float ox = COORD_NORMALISE(scr_pos[v].x / scr_pos[v].z) * pipeline->mScrWidth;
            float oy = COORD_NORMALISE(scr_pos[v].y / scr_pos[v].z) * pipeline->mScrHeight;
#ifdef LOG_DRAW
            printf("[%d] -> (%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f,%.1f) -> (%d,%d)\n", ind[v], f[layout.mPosStart], f[layout.mPosStart + 1], f[layout.mPosStart + 1], 1.0f, p[v].pos.x, p[v].pos.y, p[v].pos.z, p[v].pos.w, ox, oy);
#endif
            scr_pos[v].x = ox;
            scr_pos[v].y = oy;

        }


        //SR_RasterizeTris(pipeline, &p[0], &p[1], &p[2]);
        //SR_RasterizeLines(pipeline, &p[0], &p[1], &p[2]);
        pipeline->mRasterizeFunc(pipeline, &p[0], &p[1], &p[2], &scr_pos[0], &scr_pos[1], &scr_pos[2]);
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