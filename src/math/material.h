#pragma once
#include "device_types.h"
#include "vec3.h"

enum class MaterialType
{
  LAMBERTIAN,
  METAL,
  DIELECTRIC
};

struct Material
{
  MaterialType type;
  Vec3 albedo;
  float fuzz;
  float ir;

  __device__ __host__ Material() {}

  __device__ __host__ Material(MaterialType t, const Vec3 &a, float f, float i)
      : type(t), albedo(a), fuzz(f), ir(i)
  {
  }
};
