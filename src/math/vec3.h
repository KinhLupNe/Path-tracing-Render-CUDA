#pragma once
#include "device_types.h"

#include <cmath>
#include <cuda_runtime.h>

#include <iostream>
#include <curand_kernel.h>
#include <sal.h>
class alignas(16) Vec3
{
public:
  float e[3];
  __device__ __host__ inline Vec3()
  {
    e[0] = 0;
    e[1] = 0;
    e[2] = 0;
  }
  __device__ __host__ inline Vec3(float e0, float e1, float e2)
  {
    e[0] = e0;
    e[1] = e1;
    e[2] = e2;
  }
  __device__ __host__ inline float x() const
  {
    return e[0];
  }
  __device__ __host__ inline float y() const
  {
    return e[1];
  }
  __device__ __host__ inline float z() const
  {
    return e[2];
  }

  __device__ __host__ inline Vec3 operator-() const
  {
    return Vec3(-e[0], -e[1], -e[2]);
  }

  // cong 2 vector
  __device__ __host__ inline Vec3 operator+(const Vec3 &v) const
  {
    return Vec3(this->e[0] + v.e[0], this->e[1] + v.e[1], this->e[2] + v.e[2]);
  }

  __device__ __host__ inline Vec3 operator-(const Vec3 &v) const
  {
    return Vec3(this->e[0] - v.e[0], this->e[1] - v.e[1], this->e[2] - v.e[2]);
  }

  __device__ __host__ inline Vec3 operator*(const Vec3 &v) const
  {
    return Vec3(this->e[0] * v.e[0], this->e[1] * v.e[1], this->e[2] * v.e[2]);
  }
  // nhan vector voi so thuc
  __device__ __host__ inline Vec3 operator*(float t) const
  {
    return Vec3(this->e[0] * t, this->e[1] * t, this->e[2] * t);
  }
  // chia vector cho so thuc
  __device__ __host__ inline Vec3 operator/(float t) const
  {
    return (*this) * (1.0f / t);
  }
  // chieu dai binh phuong
  __device__ __host__ inline float length_squared() const
  {
    return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  }
  // chieu dai vector
  __device__ __host__ inline float length() const
  {
    return sqrtf(length_squared());
  }
};

// ham tien cho

__device__ __host__ inline Vec3 operator*(float t, const Vec3 &v)
{
  return v * t;
}
// dot product
__device__ __host__ inline float dot(const Vec3 &u, const Vec3 &v)
{
  return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

// cross product

__device__ __host__ inline Vec3 cross(const Vec3 &u, const Vec3 &v)
{
  return Vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1], u.e[2] * v.e[0] - u.e[0] * v.e[2],
              u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

// Chuan hoa vector

__device__ __host__ inline Vec3 unit_vector(const Vec3 &v)
{
  return v / v.length();
}

__device__ __host__ inline Vec3 reflect(const Vec3 &v, const Vec3 &n)
{
  return v - 2.0f * dot(v, n) * n;
}

// sinh so ngau nhien
__device__ inline float random_float(curandState *state)
{
  return curand_uniform(state);
}

__device__ inline float random_float(float min, float max, curandState *state)
{
  return min + (max - min) * random_float(state);
}

// sinh vector ngau nhien trong vofng tro don vi
__device__ inline Vec3 random_in_unit_vector(curandState *state)
{
  while (true)
  {
    Vec3 p = Vec3(random_float(-1.0f, 1.0f, state), random_float(-1.0f, 1.0f, state),
                  random_float(-1.0f, 1.0f, state));
    if (p.length() >= 1.0f)
      continue;
    return p;
  }
}
// sinh vector tren be mat khoi cau
__device__ inline Vec3 random_unit_vector(curandState *state)
{
  return unit_vector(random_in_unit_vector(state));
}

// Định danh phụ cho code dễ hiểu
using Point3 = Vec3;
using Color = Vec3;
