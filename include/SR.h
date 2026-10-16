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

    SR_LAYOUT_NORMAL_F
    
} SR_LAYOUT_PROP;

typedef enum SR_ERROR{
    SR_ERR_NONE = 0,
    SR_ERR_TOO_LONG,
} SR_ERROR;

#define SR_MAX_TEXTURES 16

typedef struct SR_Pipeline SR_Pipeline;

typedef struct SR_Texture{
    col4* pixels; //stored as RGBA for fastest process time, since we cant afford the speed to switch between tex formats at runtime
    int w;
    int h;
} SR_Texture;

typedef struct {
    float4 pos; //position
    float3 normal; //normal
    col4 col; //colour rgba
    float2 uv; //UV coords
} SR_Point;

typedef struct {
    float3 lightDir;
    float3 viewPos;
} SR_RenderProps;


/** 
* The function signature for a vertex shader function
* 
* @param modelview The modelview matrix
* @param persp The projection matrix
* @param pixel The worldspace vertex to be transformed
* 
* @return The normalised screen space position of the vertex
* 
* @see SR_PixelFunc
* @see SR_BindVertexFunc
*/
typedef float4 (*SR_VertexFunc)(mat4x4* modelview, mat4x4* persp, SR_Point* pixel);


/** 
* The function signature for a pixel shader function
*
* @param props A collection of properties shared with every pixel
* @param pixel The position of the vertex
* @param textures An array of texture slots, bound with a call to SR_BindTexture
* @return The 4 byte RGBA colour of the pixel
*
* @see SR_VertexFunc
* @see SR_BindPixelFunc
* @see SR_BindTexture
*/
typedef col4 (*SR_PixelFunc)(SR_RenderProps* props, SR_Point* pixel, SR_Texture* textures[SR_MAX_TEXTURES]);

/** Creates a pipeline object 
 *
 * @param initialTarget The initial render target to render too. This can be NULL
 * @param w The width of initialTarget in pixels
 * @param h The height of initialTarget in pixels
 * @param format The colour format of initialTarget. This can be SR_COLOUR_EMPTY
 * 
 * @return The created pipeline object, or NULL on failure.
 * 
 * @see SR_DestroyPipeline
 * @see SR_UpdateRenderTarget
 */
SR_DLLFUNC SR_Pipeline* SR_CreatePipeline(void* initalTarget, int w, int h, SR_COLOURFORMAT format);

/** Destroys a pipeline object
 * @param pipeline The pipeline to destroy
 * @see SR_CreatePipeline
 */
SR_DLLFUNC void SR_DestroyPipeline(SR_Pipeline* pipeline);

/** Creates a texture object
 * 
 * This function copies a texture from the pointer at data, and stores it as RGBA internally.
 * 
 * @param data The raw image data
 * @param w The width of data in pixels
 * @param h The height of data in pixels
 * @param fmt The colour format of data
 * @return
 * @see SR_DestroyTexture
 */
SR_DLLFUNC SR_Texture* SR_CreateTexture(void* data, int w, int h, SR_COLOURFORMAT fmt);

/** Destroys a texture
 * @param texture The texture to destroy
 * @see SR_CreateTexture
 */
SR_DLLFUNC void SR_DestroyTexture(SR_Texture* texture);

/** Enables/Disables the Depth Buffer
 * 
 * If this function is enabling a previously disabled depth buffer, it will allocate immediately. 
 * If no change happens, such as the pipeline already having a depth buffer enabled, this function does nothing.
 * 
 * @param pipeline The target pipeline
 * @param enabled The value to set
 */
SR_DLLFUNC void SR_ToggleDepthTest(SR_Pipeline* pipeline, int enabled);

/** Sets the current culling mode
 *
 * @param pipeline The target pipeline
 * @param mode The new culling mode
 * 
 * @see SR_CULLMODE
 */
SR_DLLFUNC void SR_SetCullMode(SR_Pipeline* pipeline, enum SR_CULLMODE mode);

/** Updates the render target
 *
 * This function works the same way as the initial data as SR_CreatePipeline.
 * It also does not allocate any memory unless the depth buffer is set, and the new width and height are DIFFERENT than the last values.
 * Even if the depth buffer is enabled, if the width and height stay the same, it will not recreate the buffer.
 * 
 * @param target The new render target to render to.
 * @param w The width of target in pixels
 * @param h The height of target in pixels
 * @param format The colour format of target.
 *
 * @return 0 on success, else an SR_ERROR code
 *
 * @see SR_CreatePipeline
 * @see SR_ERROR
 */
SR_DLLFUNC SR_ERROR SR_UpdateRenderTarget(SR_Pipeline* pipeline, void* target, int w, int h, SR_COLOURFORMAT format);

/** Binds a Vertex shader function to the pipeline
 * 
 * @param pipeline The target pipeline
 * @param func The new vertex function, or NULL for the default.
 *
 * @see SR_VertexFunc
 */
SR_DLLFUNC void SR_BindVertexFunc(SR_Pipeline* pipeline, SR_VertexFunc func);

/** Binds a Pixel shader function to the pipeline
 *
 * @param pipeline The target pipeline
 * @param func The new pixel function, or NULL for the default.
 *
 * @see SR_PixelFunc
 */
SR_DLLFUNC void SR_BindPixelFunc(SR_Pipeline* pipeline, SR_PixelFunc func);

/** Binds a set of input layouts to the pipeline
 *
 * The props parameter contains an array of enum values that, in order, relate to the vertex format of the input vertices.
 * For example, a props of [SR_LAYOUT_POS_XYZ_F, SR_LAYOUT_COLOUR_RGB_F, SR_LAYOUT_NORMAL_F] means that the input layout has
 * the first three floats as the position, the next three as the RGB colour, and the last three as the normal, making the format 9 floats long.
 * 
 * @param pipeline The target pipeline
 * @param props An array of SR_LAYOUT_PROP
 * @param count The number of elements in props
 *
 * @return 0 on success, else an SR_ERROR code
 * 
 * @see SR_LAYOUT_PROP
 */
SR_DLLFUNC SR_ERROR SR_BindLayout(SR_Pipeline* pipeline, SR_LAYOUT_PROP* props, int count);

/** Binds a matrix to the modelview matrix passed into the vertex func
 * 
 * @param pipeline The target pipeline
 * @param matrix The matrix to bind
 *
 * @see SR_VertexFunc
 */
SR_DLLFUNC void SR_BindModelview(SR_Pipeline* pipeline, mat4x4* matrix);

/** Binds a texture to a slot in the pipeline passed into the pixel func
 *
 * @param pipeline The target pipeline
 * @param texture The texture to bind
 * @param slot The target texture slot
 *
 * @see SR_PixelFunc
 */
SR_DLLFUNC SR_ERROR SR_BindTexture(SR_Pipeline* pipeline, SR_Texture* texture, int slot);

/** Creates and binds a perspective matrix to the persp matrix passed into the vertex func
 *
 * @param pipeline The target pipeline
 * @param near The near plane
 * @param far The far plane
 * @param aspect The aspect ratio of the resulting matrix (screen width/height)
 * @param FOV The field of view of the resulting matrix
 * @see SR_Persp
 */
SR_DLLFUNC void SR_PerspFOV(SR_Pipeline* pipeline, float near, float far, float aspect, float FOV);

/** Creates and binds a perspective matrix to the persp matrix passed into the vertex func
 *
 * Use SR_PerspFOV over this whenever possible
 * 
 * @param pipeline The target pipeline
 * @param near The near plane
 * @param far The far plane
 * @param left The left plane
 * @param right The right plane
 * @param top The top plane
 * @param bottom The bottom plane
 * @see SR_PerspFOV
 */
SR_DLLFUNC void SR_Persp(SR_Pipeline* pipeline, float near, float far, float left, float right, float top, float bottom);

/** Draws some geometry to the render target
 *
 * @param pipeline The target pipeline
 * @param vertices A pointer to an array of vertices, with the format specified by a prior call to SR_BindLayout
 * @param indices A pointer to an array of triangle indices into vertices
 * @param count The number of triangles to draw, usually sizeof(indices)/3
 * 
 * @see SR_BindLayout
 */
SR_DLLFUNC void SR_DrawIndexed(SR_Pipeline* pipeline, float* vertices, int* indices, unsigned long long count);

/** Clears the current render target
 *
 * @param pipeline The target pipeline
 * @param c The colour to clear to
 * @see SR_ClearDepthBuffer
 */
SR_DLLFUNC void SR_Clear(SR_Pipeline* pipeline, fcol4 c);

/** Clears the current depth buffer
 *
 * If the depth buffer is not enabled, this function does nothing.
 * 
 * @param pipeline The target pipeline
 *
 * @see SR_Clear
 */
SR_DLLFUNC void SR_ClearDepthBuffer(SR_Pipeline* pipeline);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SR_HEADER_