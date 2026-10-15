#ifndef _SR_TYPE_H_
#define _SR_TYPE_H_

typedef struct{
    float x, y, z, w;
} float4;

typedef struct{
    float r, g, b, a;
} fcol4;

typedef struct{
    unsigned char r, g, b, a;
} col4;

typedef struct{
    float x, y;
} float2;

typedef struct{

    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
} mat4x4;

#endif