#ifndef _SR_MATRIX_H_
#define _SR_MATRIX_H_

typedef struct{
    float x, y, z ,w;
} float4;

typedef struct{

    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
} mat4x4;


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