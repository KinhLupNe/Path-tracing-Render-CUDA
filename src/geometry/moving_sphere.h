#pragma once

#include "hittable.h"
#include "../material/material.h"

class MovingSphere : public Hittable {
public:
  __device__ MovingSphere() {}
  __device__ MovingSphere(Point3 cen0, Point3 cen1, float t0, float t1, float r, Material m)
      : center0(cen0), center1(cen1), time0(t0), time1(t1), radius(r), mat(m) {};

  __device__ virtual bool hit(const Ray &r, float t_min, float t_max,
                              hit_record &rec) const override;
  
  __device__ Point3 center(float time) const;

public:
  Point3 center0, center1;
  float time0, time1;
  float radius;
  Material mat;
};

__device__ Point3 MovingSphere::center(float time) const {
    return center0 + ((time - time0) / (time1 - time0)) * (center1 - center0);
}

__device__ bool MovingSphere::hit(const Ray &r, float t_min, float t_max,
                                  hit_record &rec) const {
  Point3 current_center = center(r.time());
  Vec3 oc = r.origin() - current_center;
  float a = r.direction().length_squared();
  float half_b = dot(oc, r.direction());
  float c = oc.length_squared() - radius * radius;
  float discriminant = half_b * half_b - a * c;

  if (discriminant < 0)
    return false;
  float sqrtd = sqrt(discriminant);

  float root = (-half_b - sqrtd) / a;
  if (root < t_min || root > t_max) {
    root = (-half_b + sqrtd) / a;
    if (root < t_min || root > t_max)
      return false;
  }

  rec.t = root;
  rec.p = r.at(rec.t);
  Vec3 outward_normal = (rec.p - current_center) / radius;
  rec.normal = outward_normal;
  rec.mat = mat;

  return true;
}
