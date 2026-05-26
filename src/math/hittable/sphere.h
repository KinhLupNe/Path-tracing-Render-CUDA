#pragma once
#include "../ray.h"
#include "../vec3.h"
#include <cmath>
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

  // kiem tra tia sang co dam trung qua cau khong
  __device__ __host__ inline bool hit(const Ray &r, float t_min, float t_max,
                                      hit_record &rec) const {
    // he so cua at^2 + bt +c = 0
    Vec3 oc = r.origin() - center;
    float a = dot(r.direction(), r.direction());
    float b = 2.0f * dot(r.direction(), oc);
    float c = dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;
    // delta < 0 vo nghiem
    if (discriminant < 0)
      return false;
    // delta >=0 co nghiem
    float sqrtd = sqrtf(discriminant);

    float root = (-b - sqrtd) / (2.0f * a);
    // uu tien lay diem giao gan nhat
    if (root < t_min || root > t_max) {
      root = (-b + sqrtd) / (2.0f * a);
      if (root < t_min || root > t_max) {
        return false;
      }
    }
    rec.t = root;
    rec.p = r.at(root);
    rec.normal = (rec.p - center) / radius;
    return true;
  }
};
