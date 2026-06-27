#pragma once

#include <cuda_runtime.h>
#include "core/include/core/vector.h"

class Matrix3 {
public:
    float m[3][3];

    __host__ __device__ Matrix3() {
        m[0][0] = 1; m[0][1] = 0; m[0][2] = 0;
        m[1][0] = 0; m[1][1] = 1; m[1][2] = 0;
        m[2][0] = 0; m[2][1] = 0; m[2][2] = 1;
    }

    __host__ __device__ Vector3 operator*(const Vector3& v) const {
        return Vector3(
            m[0][0]*v.x() + m[0][1]*v.y() + m[0][2]*v.z(),
            m[1][0]*v.x() + m[1][1]*v.y() + m[1][2]*v.z(),
            m[2][0]*v.x() + m[2][1]*v.y() + m[2][2]*v.z()
        );
    }

    __host__ __device__ Matrix3 operator*(const Matrix3& b) const {
        Matrix3 result;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 3; k++)
                    result.m[i][j] += m[i][k] * b.m[k][j];
            }
        return result;
    }

    __host__ __device__ Matrix3 transpose() const {
        Matrix3 result;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                result.m[i][j] = m[j][i];
        return result;
    }

    __host__ __device__ Matrix3 T() const { return transpose(); }

    __host__ __device__ Vector3 right() const { 
        return Vector3(m[0][0], m[1][0], m[2][0]); 
    }
    __host__ __device__ Vector3 up() const { 
        return Vector3(m[0][1], m[1][1], m[2][1]); 
    }
    __host__ __device__ Vector3 forward() const { 
        return Vector3(m[0][2], m[1][2], m[2][2]); 
    }
};

__host__ __device__ inline Matrix3 rotation_x(float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    Matrix3 r;
    r.m[0][0]=1; r.m[0][1]=0;  r.m[0][2]=0;
    r.m[1][0]=0; r.m[1][1]=c;  r.m[1][2]=-s;
    r.m[2][0]=0; r.m[2][1]=s;  r.m[2][2]=c;
    return r;
}

__host__ __device__ inline Matrix3 rotation_y(float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    Matrix3 r;
    r.m[0][0]=c;  r.m[0][1]=0; r.m[0][2]=s;
    r.m[1][0]=0;  r.m[1][1]=1; r.m[1][2]=0;
    r.m[2][0]=-s; r.m[2][1]=0; r.m[2][2]=c;
    return r;
}

__host__ __device__ inline Matrix3 rotation_z(float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    Matrix3 r;
    r.m[0][0]=c;  r.m[0][1]=-s; r.m[0][2]=0;
    r.m[1][0]=s;  r.m[1][1]=c;  r.m[1][2]=0;
    r.m[2][0]=0;  r.m[2][1]=0;  r.m[2][2]=1;
    return r;
}

__host__ __device__ inline Matrix3 rotation_from_euler(Vector3 r) {
    return rotation_y(r.x()) * rotation_x(r.y()) * rotation_z(r.z());
}

