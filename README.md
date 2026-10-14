# Software Renderer

A simple software renderer written in C. It is capable of rendering to anything that can provide a pointer to a pixel array, and requires no external libraries other than the C stdlib.

## Installing

There are two ways to use this project:

1. Direct (Recommended)

- Copy the source files from the src directory to your project.
- Add the include folder to your include path

2. Header-Only

- Run gen_header_only.bat
- Add the include folder to your include path
- At the top of **ONE** source file, insert 
```
#define __SR_IMPL__
#include <SR_impl.h>
```

## Quick Start

This project was not designed to be difficult, and setting up is relatively easy.
Everything in this project is controlled by a SR_Pipeline pointer, which is aquired by calling `SR_CreatePipeline(void* initalTarget, int w, int h, SR_COLOURFORMAT format)`. `initialTarget` is a pointer to a grid of pixels for the pipeline to write to, and `w` and `h` are the size in pixels of that target. `format` is the pixel format of the render target.
`initialTarget`, `w` and `h` can also be NULL if you want to set them later.

Whenever the render target changes (resized, no longer writable, etc), you must call `SR_UpdateRenderTarget` with new values before trying to render again, as there is no way of knowing internally if the render target has changed, and thus will still try and write to the deleted target.

One more thing you need to think about is `SR_BindLayout`. This is what tells the renderer how to interpret your vertices. Elements in the `SR_LAYOUT_PROP` array passed in are, in order, the format oof the vertices. For example, the code below has it set to `{SR_LAYOUT_POS_XYZ_F, SR_LAYOUT_COLOUR_RGB_F}`,  which tells the engine that the first three values are the XYZ coordinates, and the next three are the RGB colour values. Currently, only RGB colours are implemented, and transparency not at all, so stick to `SR_LAYOUT_COLOUR_RGB_F` for now.

For a simple setup, see this code snippet:

```
    #include <SR.h>

    ...

    // Create Pipeline Object
    SR_Pipeline* pipeline = SR_CreatePipeline(NULL, 0, 0, SR_COLOUR_EMPTY);

    // Bind render target
    SR_UpdateRenderTarget(PLATFORM_DEPENDANT_SOURCE, width, height, SR_COLOUR_RGBA);

    // Setup internal state for rendering
    SR_PerspFOV(pipeline, 0.1f, 1000, width/height, 70);

    mat4x4 mv = mat4transform(1,1,1, 0,0,-5,0,0,0);
    SR_BindModelview(pipeline, &mv);

    SR_ToggleDepthTest(pipeline, 1);

    ...

    //-------------
    // Begin Render
    //-------------

    SR_Clear(pipeline, (fcol4){1,1,1,1});
    SR_ClearDepthBuffer(pipeline);

    // Bind layout of current object
    enum SR_LAYOUT_PROP props[] = {SR_LAYOUT_POS_XYZ_F, SR_LAYOUT_COLOUR_RGB_F};
    SR_BindLayout(pipeline,props,2);

    // Draw to surface!
    SR_DrawIndexed(pipeline,MY_VERTEX_LIST,MY_INDEX_LIST,MY_INDEX_COUNT);

    //Most platforms will not let you draw direct to screen, and so you will probably have to copy from your buffer to the screen here

```
