#pragma once
#include "../ray.h"
#include "../vec3.h"
#include "device_types.h"
#include "math/material.h"

struct hit_record
{
  float t;
  Point3 p;
  Vec3 normal;
  Material mat;
};

class Hittable
{
public:
  // bat cac vat lieu tu viet ham hit cho rieng no
  //  = 0 - > bắt các hàm con phải override lại, không cho phép tạo obj từ lớp
  //  có hàm này rec không có const vì mình đang cần ghi đè
  __device__ virtual bool hit(const Ray &r, float t_min, float t_max, hit_record &rec) const = 0;
};
