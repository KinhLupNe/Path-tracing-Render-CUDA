#pragma once
#include "../ray.h"
#include "../vec3.h"
#include <cuda_runtime.h>

struct hit_record {
  float t;
  Point3 p;
  Vec3 normal;
};

class Sphere {
public:
  Point3 center;
  float radius;
  __device__ __host__ inline Sphere() {}
  __device__ __host__ inline Sphere(const Point3 &center, float radius)
      : center(center), radius(radius) {}
};
