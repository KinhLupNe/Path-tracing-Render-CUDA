#pragma once
#include "vec3.h"
#include <cuda_runtime.h>

class Ray {
public:
  Point3 orig;
  Vec3 dir;
  float tm;

  __device__ __host__ inline Ray() {}
  __device__ __host__ inline Ray(const Point3 &origin, const Vec3 &direction, float time = 0.0f)
      : orig(origin), dir(direction), tm(time) {}

  __device__ __host__ inline Point3 origin() const { return orig; }

  __device__ __host__ inline Vec3 direction() const { return dir; }
  
  __device__ __host__ inline float time() const { return tm; }

  // Vi tri tia sang tai thoi diem t
  __device__ __host__ inline Point3 at(float t) const { return orig + dir * t; }
};
