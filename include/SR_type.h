#ifndef _SR_TYPE_H_
#define _SR_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    float x, y, z, w;
} float4;

typedef struct {
    float x, y, z;
} float3;

typedef struct{
    float r, g, b, a;
} fcol4;

typedef struct{
    unsigned char r, g, b;
} col3;
typedef struct {
    unsigned char r, g, b, a;
} col4;

typedef struct{
    float x, y;
} float2;


#ifdef __cplusplus
}
#endif


#endif