#ifndef _SR_HEADER_
#define _SR_HEADER_

enum SR_CULLMODE{
    SR_CULL_NONE = 0,
    SR_CULL_FRONT = 1,
    SR_CULL_BACK = -1,
};

enum SR_DRAWMODE{
    SR_DRAW_TRIS,
    SR_DRAW_POINTS,
    SR_DRAW_LINES
};

enum SR_LAYOUT_PROP{
    SR_LAYOUT_POS_XYZ_F,

    SR_LAYOUT_COLOUR_RGB_F,
    SR_LAYOUT_COLOUR_BGR_F,
    SR_LAYOUT_COLOUR_RGBA_F,
    SR_LAYOUT_COLOUR_BGRA_F,

    SR_LAYOUT_TEXCOORD_UV_F,
    
};

typedef enum SR_ERROR{
    SR_ERR_NONE = 0,
    SR_ERR_TOO_LONG,
} SR_ERROR;


typedef struct SR_Pipeline SR_Pipeline;

SR_Pipeline* SR_CreatePipeline(void* initalTarget, int w, int h);
void SR_DestroyPipeline(SR_Pipeline* pipeline);
void SR_ToggleDepthTest(SR_Pipeline* pipeline, int enabled);
SR_ERROR SR_UpdateRenderTarget(SR_Pipeline* pipeline, void* target, int w, int h);
SR_ERROR SR_BindLayout(SR_Pipeline* pipeline, enum SR_LAYOUT_PROP* props, int count );
void SR_BindModelview(SR_Pipeline* pipeline, mat4x4* matrix);
void SR_PerspFOV(SR_Pipeline* pipeline, float near, float far, float aspect, float FOV);
void SR_Persp(SR_Pipeline* pipeline, float near, float far, float left, float right, float top, float bottom);

void SR_DrawIndexed(SR_Pipeline* pipeline, float* vertices, int* indices, unsigned long long count);


#endif //_SR_HEADER_