#ifndef _SR_MATRIX_H_
#define _SR_MATRIX_H_
#include "SR_type.h"


mat4x4 mat4identity();
mat4x4 mat4translate(float x, float y, float z);
mat4x4 mat4scale(float x, float y, float z);
mat4x4 mat4rotatex(float t);
mat4x4 mat4rotatey(float t);
mat4x4 mat4rotatez(float t);
mat4x4 mat4transpose(mat4x4* m);
mat4x4 mat4mul4(mat4x4* b, mat4x4* a);
float4 mat4mul(mat4x4* mat, float4 v);

mat4x4 mat4transform(float scaleX, float scaleY, float scaleZ, float transX, float transY, float transZ, float rotX, float rotY, float rotZ);

#endif