#include "SR_matrix.h"

#include <math.h>

mat4x4 mat4identity(){
    mat4x4 m = {0};
    m.m00 = 1;
    m.m11 = 1;
    m.m22 = 1;
    m.m33 = 1;
    return m;
}

mat4x4 mat4translate(float x, float y, float z){
    mat4x4 m = {0};
    m.m00 = 1;
    m.m11 = 1;
    m.m22 = 1;
    m.m33 = 1;

    m.m03 = x;
    m.m13 = y;
    m.m23 = z;

    return m;
}
mat4x4 mat4scale(float x, float y, float z){
    mat4x4 m = {0};
    m.m00 = x;
    m.m11 = y;
    m.m22 = z;
    m.m33 = 1;

    return m;
}

mat4x4 mat4rotatex(float t){
    mat4x4 m = {0};
    m.m00 = 1;
    m.m11 = cosf(t);
    m.m12 = -sinf(t);
    m.m21 = sinf(t);
    m.m22 = cosf(t);
    m.m33 = 1;

    return m;
}
mat4x4 mat4rotatey(float t){
    mat4x4 m = {0};
    m.m00 = cosf(t);
    m.m02 = sinf(t);
    m.m11 = 1;
    m.m20 = -sinf(t);
    m.m22 = cosf(t);
    m.m33 = 1;

    return m;
}
mat4x4 mat4rotatez(float t){
    mat4x4 m = {0};

    m.m00 = cosf(t);
    m.m01 = -sinf(t);
    m.m10 = sinf(t);
    m.m11 = cosf(t);
    m.m22 = 1;
    m.m33 = 1;

    return m;
}


mat4x4 mat4transpose(mat4x4* m){
    mat4x4 r;
    r.m00 = m->m00;
    r.m01 = m->m10;
    r.m02 = m->m20;
    r.m03 = m->m30;

    r.m10 = m->m01;
    r.m11 = m->m11;
    r.m12 = m->m21;
    r.m13 = m->m31;

    r.m20 = m->m02;
    r.m21 = m->m12;
    r.m22 = m->m22;
    r.m23 = m->m32;

    r.m30 = m->m03;
    r.m31 = m->m13;
    r.m32 = m->m23;
    r.m33 = m->m33;
    return r;
}

mat4x4 mat4mul4(mat4x4* b, mat4x4* a){
    mat4x4 m;
    m.m01 = (a->m00 * b->m01) + (a->m01 * b->m11) + (a->m02 * b->m21) + (a->m03 * b->m31);
    m.m00 = (a->m00 * b->m00) + (a->m01 * b->m10) + (a->m02 * b->m20) + (a->m03 * b->m30);
    m.m02 = (a->m00 * b->m02) + (a->m01 * b->m12) + (a->m02 * b->m22) + (a->m03 * b->m32);
    m.m03 = (a->m00 * b->m03) + (a->m01 * b->m13) + (a->m02 * b->m23) + (a->m03 * b->m33);
    m.m10 = (a->m10 * b->m00) + (a->m11 * b->m10) + (a->m12 * b->m20) + (a->m13 * b->m30);
    m.m11 = (a->m10 * b->m01) + (a->m11 * b->m11) + (a->m12 * b->m21) + (a->m13 * b->m31);
    m.m12 = (a->m10 * b->m02) + (a->m11 * b->m12) + (a->m12 * b->m22) + (a->m13 * b->m32);
    m.m13 = (a->m10 * b->m03) + (a->m11 * b->m13) + (a->m12 * b->m23) + (a->m13 * b->m33);
    m.m20 = (a->m20 * b->m00) + (a->m21 * b->m10) + (a->m22 * b->m20) + (a->m23 * b->m30);
    m.m21 = (a->m20 * b->m01) + (a->m21 * b->m11) + (a->m22 * b->m21) + (a->m23 * b->m31);
    m.m22 = (a->m20 * b->m02) + (a->m21 * b->m12) + (a->m22 * b->m22) + (a->m23 * b->m32);
    m.m23 = (a->m20 * b->m03) + (a->m21 * b->m13) + (a->m22 * b->m23) + (a->m23 * b->m33);
    m.m30 = (a->m30 * b->m00) + (a->m31 * b->m10) + (a->m32 * b->m20) + (a->m33 * b->m30);
    m.m31 = (a->m30 * b->m01) + (a->m31 * b->m11) + (a->m32 * b->m21) + (a->m33 * b->m31);
    m.m32 = (a->m30 * b->m02) + (a->m31 * b->m12) + (a->m32 * b->m22) + (a->m33 * b->m32);
    m.m33 = (a->m30 * b->m03) + (a->m31 * b->m13) + (a->m32 * b->m23) + (a->m33 * b->m33);
    return m;
}

float4 mat4mul(mat4x4* mat, float4 v){
    float4 res;
    res.x =  (mat->m00 * v.x) + (mat->m01 * v.y) + (mat->m02 * v.z)+ (mat->m03 * v.w);
    res.y =  (mat->m10 * v.x) + (mat->m11 * v.y) + (mat->m12 * v.z)+ (mat->m13 * v.w);
    res.z =  (mat->m20 * v.x) + (mat->m21 * v.y) + (mat->m22 * v.z)+ (mat->m23 * v.w);
    res.w =  (mat->m30 * v.x) + (mat->m31 * v.y) + (mat->m32 * v.z)+ (mat->m33 * v.w);
    return res;
}



mat4x4 mat4transform(float scaleX, float scaleY, float scaleZ, float transX, float transY, float transZ, float rotX, float rotY, float rotZ){

    mat4x4 scale = mat4scale(scaleX, scaleY, scaleZ);
    mat4x4 translation = mat4translate(transX, transY, transZ);
    mat4x4 rotateX = mat4rotatex(rotX);
    mat4x4 rotateY = mat4rotatey(rotY);
    mat4x4 rotateZ = mat4rotatez(rotZ);

    mat4x4 m = mat4mul4(&scale, &rotateY);
    m = mat4mul4(&m, &rotateX);
    m = mat4mul4(&m, &rotateZ);
    m = mat4mul4(&m, &translation);
    return m;
}