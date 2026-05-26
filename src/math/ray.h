#pragma once
#include "vec3.h"
#include <cuda_runtime.h>

class Ray {
public:
  Point3 orig;
  Vec3 dir;

  __device__ __host__ inline Ray() {}
  __device__ __host__ inline Ray(const Point3 &origin, const Vec3 &direction)
      : orig(origin), dir(direction) {}

  __device__ __host__ inline Point3 origin() const { return orig; }

  __device__ __host__ inline Vec3 direction() const { return dir; }
  // Vi tri tia sang tai thoi diem t
  __device__ __host__ inline Point3 at(float t) const { return orig + dir * t; }
};
