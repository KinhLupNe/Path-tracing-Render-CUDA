#pragma once
#include "hittable.h"

#include <cuda_runtime.h>
class HittableList : public Hittable
{
public:
  // * Mang cac con tro , * do du lieu dc luu heap, khi truyen vao can truyen con tro
  Hittable **list;
  int list_size;

  __device__ __host__ inline HittableList(){};
  __device__ __host__ inline HittableList(Hittable **l, int n)
  {
    list = l;
    list_size = n;
  }

  // duyet cac hittable co trong list
  __device__ bool virtual hit(const Ray &r, float t_min, float t_max,
                              hit_record &rec) const override
  {
    bool isHit = false;
    float closet = t_max;
    hit_record temp_record;
    // duyet qua danh sach
    for (int i = 0; i < list_size; i++)
    {
      // trung thi gan tmax = t ; tifm cho ra gan nhat
      if (list[i]->hit(r, t_min, closet, temp_record))
      {
        isHit = true;
        rec = temp_record;
        closet = temp_record.t;
      }
    }

    return isHit;
  }
};
