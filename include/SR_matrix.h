#ifndef _SR_MATRIX_H_
#define _SR_MATRIX_H_
#include "SR_type.h"
#include "SR_internal.h"

#ifdef __cplusplus
extern "C" {
#endif
SR_DLLFUNC mat4x4 mat4identity();
SR_DLLFUNC mat4x4 mat4translate(float x, float y, float z);
SR_DLLFUNC mat4x4 mat4scale(float x, float y, float z);
SR_DLLFUNC mat4x4 mat4rotatex(float t);
SR_DLLFUNC mat4x4 mat4rotatey(float t);
SR_DLLFUNC mat4x4 mat4rotatez(float t);
SR_DLLFUNC mat4x4 mat4transpose(mat4x4* m);
SR_DLLFUNC mat4x4 mat4mul4(mat4x4* b, mat4x4* a);
SR_DLLFUNC float4 mat4mul(mat4x4* mat, float4 v);
SR_DLLFUNC float3 mat4mul3(mat4x4* mat, float3 v);

SR_DLLFUNC mat4x4 mat4transform(float scaleX, float scaleY, float scaleZ, float transX, float transY, float transZ, float rotX, float rotY, float rotZ);

#ifdef __cplusplus
}
#endif
#endif