#include "SR.h"
#include "SR_matrix.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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
    float4 *px;
} SR_DestBuf;

#define SR_MAX_PIPELINE_PROPS 16
typedef struct {
    enum SR_LAYOUT_PROP mCurProps[SR_MAX_PIPELINE_PROPS];
    int mPropCount;
} SR_VertexLayout;

typedef struct {
    float x, y, z, w; //position
    float u, v; //UV coords
    float r,g,b,a; //colour
} SR_Point;


typedef struct SR_Pipeline{
    //pipeline funcs
    float4 (*mVertexFunc)(mat4x4* modelview, mat4x4* persp, float x, float y, float z);
    float4 (*mPixelFunc)(SR_Point* pixel);
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
float4 SR_DefaultPixelFunc(SR_Point* p);
void SR_RasterizeTris(SR_Pipeline* pipeline, SR_Point* p1, SR_Point* p2, SR_Point* p3);

SR_Pipeline* SR_CreatePipeline(void* initalTarget, int w, int h){

    SR_Pipeline* pipeline = malloc(sizeof(SR_Pipeline));

    //bind initial data
    pipeline->mDest.px = (float4*)initalTarget;
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

SR_ERROR SR_UpdateRenderTarget(SR_Pipeline* pipeline, void* target, int w, int h){
    pipeline->mDest.px = (float4*)target;
    pipeline->mScrWidth = w;
    pipeline->mScrHeight = h;

    if(pipeline->mDepthTest){
        _SR_AllocDepthBuf(pipeline);
    }
    return SR_ERR_NONE;
}

SR_ERROR SR_BindLayout(SR_Pipeline* pipeline, enum SR_LAYOUT_PROP* props, int count ){
    if(count > SR_MAX_PIPELINE_PROPS){
        pipeline->mVertexLayout.mPropCount = 0;
        return SR_ERR_TOO_LONG;
    }
    
    //copy props over
    for (int i = 0; i < count; i++)
    {
        pipeline->mVertexLayout.mCurProps[i] = props[i];
    }
    pipeline->mVertexLayout.mPropCount = count;
    
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
float4 SR_DefaultVertexFunc(mat4x4* modelview, mat4x4* persp, float x, float y, float z){
    float4 p = mat4mul(modelview, (float4){x,y,z,1.0f});
    p = mat4mul(persp,p);
    return p;
}

float4 SR_DefaultPixelFunc(SR_Point* p){
    return (float4){CLAMP(p->b,0,1) * 255,CLAMP(p->g,0,1) * 255, CLAMP(p->r,0,1) * 255, 255};
}


void SR_DrawPoint(SR_Pipeline* pipeline, int x, int y, SR_Point* p){
    if(!pipeline->mDest.px){
        return;
    }
    if (x >= 0 && x < pipeline->mScrWidth && y >= 0 && y < pipeline->mScrHeight){
        pipeline->mDest.px[x + (y * pipeline->mScrWidth)] = pipeline->mPixelFunc(p);
    }
}


void SR_RasterizeTris(
    SR_Pipeline* pipeline,
    SR_Point* p1, 
    SR_Point* p2,  
    SR_Point* p3){

    int min_x = MIN(MIN(p1->x, p2->x), p3->x);
    int max_x = MAX(MAX(p1->x, p2->x), p3->x);
    int min_y = MIN(MIN(p1->x, p2->y), p3->z);
    int max_y = MAX(MAX(p1->x, p2->y), p3->z);

    float f = (p1->x *(p2->y - p3->y)) + (p2->x *(p3->y - p1->y)) + (p3->x *(p1->y - p2->y));

    f = 1/f; //invert

    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;

    m00 = (p2->x*p3->y) - (p3->x*p2->y);
    m10 = (p3->x*p1->y) - (p1->x*p3->x);
    m20 = (p1->x*p2->y) - (p2->x*p1->y);

    m01 = p2->y - p3->y;
    m11 = p3->y - p1->y;
    m21 = p1->y - p2->y;
    
    m02 = p3->x - p2->x;
    m12 = p1->x - p3->x;
    m22 = p2->x - p1->x;

    //(B - A) x (C - A)
    //calculate the z property of the cross product to get the winding of the triangle
    //x and y cancel out to 0 when z is 0 (ie a flat triangle)
    

    if(pipeline->mCullmode != 0){ //cullmode of zero means dont cull
        int winding = (-(p2->y - p1->y) * (p3->x - p1->x)) + ((p2->x - p1->x) * (p3->y - p1->y));

        //printf("Winding = %d\n",winding);
        //do some bitwise trickery to determine wether to cull or not
        if((winding ^ pipeline->mCullmode) >= 0){
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

                SR_Point p;
                // first calculate the depth and throw this pixel away if its not going to be visible
                // dont calculate everything first if we dont have to
                p.w = (p1->w * bx) + (p2->w * by) + (p3->w * bz);
                
                //only check for depth if there is a depth buffer bound
                if(pipeline->mDepthBuffer){
                    if(p.w > pipeline->mDepthBuffer[i + j*pipeline->mScrWidth]){
                        continue;
                    }
                    pipeline->mDepthBuffer[i + j*pipeline->mScrWidth] = p.w;
                }

                //this pixel is going to be rendered, calculate eveything else
                p.x = (p1->x * bx) + (p2->x * by) + (p3->x * bz);
                p.y = (p1->y * bx) + (p2->y * by) + (p3->y * bz);
                p.z = (p1->z * bx) + (p2->z * by) + (p3->z * bz);


                p.r = (p1->r * bx) + (p2->r * by) + (p3->r * bz);
                p.g = (p1->g * bx) + (p2->g * by) + (p3->g * bz);
                p.b = (p1->b * bx) + (p2->b * by) + (p3->b * bz);
                p.a = 1; //no transparency support yet so alpha is hardcoded to 1


                SR_DrawPoint(pipeline, i,j, &p);
            }
        }
    }
    
}


void SR_DrawIndexed(SR_Pipeline* pipeline, float* vertices, int* indices, unsigned long long count){

#define FORMAT_LENGTH 6
    int c = 0;

    //extract these before the loop so we dont have to access a pointer in the tight loop
    mat4x4* persp = &pipeline->mPersp;
    mat4x4* modelview = &pipeline->mModelView;


    

    count = count * 3;
    for (unsigned long long i = 0; i < count; i += 3)
    {
        
        SR_Point p1, p2, p3;

        int i1 = (indices[(i)+0] * FORMAT_LENGTH);
        int i2 = (indices[(i)+1] * FORMAT_LENGTH);
        int i3 = (indices[(i)+2] * FORMAT_LENGTH);



        for (int i = 0; i < pipeline->mVertexLayout.mPropCount; i++)
        {
            switch (pipeline->mVertexLayout.mCurProps[i])
            {
            case SR_LAYOUT_POS_XYZ_F:
                p1.x = vertices[i1++];
                p1.y = vertices[i1++];
                p1.z = vertices[i1++];

                p2.x = vertices[i2++];
                p2.y = vertices[i2++];
                p2.z = vertices[i2++];

                p3.x = vertices[i3++];
                p3.y = vertices[i3++];
                p3.z = vertices[i3++];
                break;
            case SR_LAYOUT_COLOUR_RGB_F:
                p1.r = vertices[i1++];
                p1.g = vertices[i1++];
                p1.b = vertices[i1++];
                p1.a = 1;

                p2.r = vertices[i2++];
                p2.g = vertices[i2++];
                p2.b = vertices[i2++];
                p2.a = 1;

                p3.r = vertices[i3++];
                p3.g = vertices[i3++];
                p3.b = vertices[i3++];
                p3.a = 1;
                break;
            
            default:
                break;
            }
        }

        //we need the position separate so we can process them in the vertex shader
        float x1 = vertices[i1 + 0];
        float y1 = vertices[i1 + 1];
        float z1 = vertices[i1 + 2];
        p1.r = vertices[i1 + 3];
        p1.g = vertices[i1 + 4];
        p1.b = vertices[i1 + 5];
        p1.a = 1;

        float x2 = vertices[i2 + 0];
        float y2 = vertices[i2 + 1];
        float z2 = vertices[i2 + 2];
        p2.r = vertices[i2 + 3];
        p2.g = vertices[i2 + 4];
        p2.b = vertices[i2 + 5];
        p2.a = 1;

        float x3 = vertices[i3 + 0];
        float y3 = vertices[i3 + 1];
        float z3 = vertices[i3 + 2];
        p3.r = vertices[i3 + 3];
        p3.g = vertices[i3 + 4];
        p3.b = vertices[i3 + 5];
        p1.a = 1;
        

        //printf("tri = (%.1f,%.1f,%.1f) (%.1f,%.1f,%.1f) (%.1f,%.1f,%.1f)\n",x1,y1,z1,x2,y2,z2,x3,y3,z3);

        //vertex shader

        

        float4 v1 = pipeline->mVertexFunc(modelview, persp, x1,y1,z1);
        float4 v2 = pipeline->mVertexFunc(modelview, persp, x2,y2,z2);
        float4 v3 = pipeline->mVertexFunc(modelview, persp, x3,y3,z3);


        //finangling to fill 4 floats at once
        ((float4*)&p1.x)[0] = v1;
        ((float4*)&p2.x)[0] = v2;
        ((float4*)&p3.x)[0] = v3;

        //scale by Z to put on correct place on screen
        p1.x /= p1.z;
        p1.y /= p1.z;

        p2.x /= p2.z;
        p2.y /= p2.z;

        p3.x /= p3.z;
        p3.y /= p3.z;




        //printf("(%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f,%.1f) -> (%.1f,%.1f)\n", x1,y1,z1,1.0f, p1.x,p1.y,p1.z,p1.w, p11.x, p11.y, p11.z, p11.w, p11.x/p11.z,p11.y/p11.z);

        pipeline->mRasterizeFunc(pipeline, &p1, &p2, &p3);
        c++;
    }

    //printf("rendered %d tris\n",c);
    
}