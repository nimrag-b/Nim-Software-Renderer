#ifndef _SR_HEADER_
#define _SR_HEADER_

#include "SR_matrix.h"
#include "SR_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    SR_COLOUR_EMPTY,
    SR_COLOUR_RGB24,
    SR_COLOUR_RGBA32,
    SR_COLOUR_BGRA32
} SR_COLOURFORMAT;

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

typedef enum SR_LAYOUT_PROP{
    SR_LAYOUT_POS_XYZ_F,

    SR_LAYOUT_COLOUR_RGB_F,
    SR_LAYOUT_COLOUR_BGR_F,
    SR_LAYOUT_COLOUR_RGBA_F,
    SR_LAYOUT_COLOUR_BGRA_F,

    SR_LAYOUT_TEXCOORD_UV_F,
    
} SR_LAYOUT_PROP;

typedef enum SR_ERROR{
    SR_ERR_NONE = 0,
    SR_ERR_TOO_LONG,
} SR_ERROR;

#define SR_MAX_TEXTURES 16

typedef struct SR_Pipeline SR_Pipeline;

typedef struct SR_Texture{
    SR_COLOURFORMAT format;
    void* pixels;
    int fmtLength;
    int w;
    int h;
} SR_Texture;

SR_DLLFUNC SR_Pipeline* SR_CreatePipeline(void* initalTarget, int w, int h, SR_COLOURFORMAT format);
SR_DLLFUNC void SR_DestroyPipeline(SR_Pipeline* pipeline);

SR_DLLFUNC SR_Texture* SR_CreateTexture(void* data, int w, int h, SR_COLOURFORMAT fmt);
SR_DLLFUNC void SR_DestroyTexture(SR_Texture* texture);


SR_DLLFUNC void SR_ToggleDepthTest(SR_Pipeline* pipeline, int enabled);
SR_DLLFUNC void SR_SetCullMode(SR_Pipeline* pipeline, enum SR_CULLMODE mode);

SR_DLLFUNC SR_ERROR SR_UpdateRenderTarget(SR_Pipeline* pipeline, void* target, int w, int h, SR_COLOURFORMAT format);

SR_DLLFUNC SR_ERROR SR_BindLayout(SR_Pipeline* pipeline, enum SR_LAYOUT_PROP* props, int count);
SR_DLLFUNC void SR_BindModelview(SR_Pipeline* pipeline, mat4x4* matrix);
SR_DLLFUNC SR_ERROR SR_BindTexture(SR_Pipeline* pipeline, SR_Texture* texture, int slot);

SR_DLLFUNC void SR_PerspFOV(SR_Pipeline* pipeline, float near, float far, float aspect, float FOV);
SR_DLLFUNC void SR_Persp(SR_Pipeline* pipeline, float near, float far, float left, float right, float top, float bottom);

SR_DLLFUNC void SR_DrawIndexed(SR_Pipeline* pipeline, float* vertices, int* indices, unsigned long long count);

SR_DLLFUNC void SR_Clear(SR_Pipeline* pipeline, fcol4 c);
SR_DLLFUNC void SR_ClearDepthBuffer(SR_Pipeline* pipeline);


SR_DLLFUNC long long SR_GetTicks();
SR_DLLFUNC long long SR_TicksPerSecond();


#ifdef __cplusplus
}
#endif

#endif //_SR_HEADER_