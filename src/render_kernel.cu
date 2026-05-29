#include "math/hittable/hittable.h"
#include "math/hittable/sphere.h"
#include "math/material.h"
#include "math/ray.h"
#include "math/vec3.h"
#include "render_kernel.h"

#include <cmath>
#include <cuda_runtime_api.h>
#include <curand_kernel.h>
#include <curand_uniform.h>
#include "math/hittable/hittable_list.h"

// schlick appro -> reflect coef -> he so nay dai dien cho ti le tia phan xa/ tia toi
__device__ inline float reflectionSchlickA(float cosine, float ir)
{
  float r = (1 - ir) / (1 + ir);
  r = r * r;
  return r + (1 - r) * powf(1 - cosine, 5);
}
// input la tia vao, ouput : tan xa hay khong ? co tra ve scattered attenuation
__device__ bool scatter(const Ray &r_in, const hit_record &rec, Vec3 &attenuation, Ray &scattered,
                        curandState *local_rand_state)
{
  switch (rec.mat.type)
  {
  case MaterialType::LAMBERTIAN:
  {
    Vec3 scatter_direction = rec.normal + random_unit_vector(local_rand_state);
    scattered = Ray(rec.p, scatter_direction);
    attenuation = rec.mat.albedo;
    return (dot(scattered.direction(), rec.normal) > 0.0f);
  }
  case MaterialType::METAL:
  {
    Vec3 reflected = reflect(r_in.direction(), rec.normal);

    // fuzz: kim loai nham
    scattered = Ray(rec.p, reflected + random_in_unit_vector(local_rand_state) * rec.mat.fuzz);
    attenuation = rec.mat.albedo;
    // se co tia sang dam vao trong vat the, chi tiep tuc khi phan ra ngoai
    return (dot(scattered.direction(), rec.normal) > 0.0f);
  }
  case MaterialType::DIELECTRIC:
  {
    attenuation = rec.mat.albedo;
    // di từ môi trường vào quả cầu , hay cầu đi ra ngoài môi trường
    float a = dot(rec.normal, r_in.direction()) < 0 ? (1.0f / rec.mat.ir) : rec.mat.ir;
    Vec3 out_normal = a == rec.mat.ir ? -rec.normal : rec.normal;
    float costheta = fabs(dot(rec.normal, unit_vector(r_in.direction())));
    bool isRefract = a * sqrtf(1.0f - costheta * costheta) > 1.0f ? false : true;
    Vec3 scatter_direction;
    if (!isRefract ||
        reflectionSchlickA(costheta, rec.mat.ir) > random_float(0.0f, 1.0f, local_rand_state))
    {
      scatter_direction = reflect(r_in.direction(), out_normal);
    }
    else
    {
      scatter_direction = refract(unit_vector(r_in.direction()), a, out_normal);
    }
    scattered = Ray(rec.p, unit_vector(scatter_direction));
    return true;
  }
  default:
    return false;
  }
}
// tinh mau cho 1 pixel
__device__ Color ray_color(const Ray &r, Hittable **world, curandState *local_rand_state)
{
  Ray cur_ray = r;
  Color cur_attenuation(1.0f, 1.0f, 1.0f);

  for (int i = 0; i < 50; i++)
  {
    hit_record rec;
    if ((*world)->hit(cur_ray, 0.001f, 10000.0f, rec))
    {
      Ray scattered;
      Vec3 attenuation;
      if (scatter(cur_ray, rec, attenuation, scattered, local_rand_state))
      {
        // Wn = Beta *Wn-1
        // Ln = Ln-1+ Le * Wn. do vat the ko tu phat xa , Le = 0
        cur_attenuation = cur_attenuation * attenuation;
        cur_ray = scattered;
      }
      else
      {
        // khong tan xa thi cook
        return Color(0.0f, 0.0f, 0.0f);
      }
    }
    else
    {
      Vec3 unit_direction = unit_vector(r.direction());
      float a = 0.5f * (unit_direction.y() + 1.0f);
      Color sky_color = (1.0f - a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 0.1f);
      // Ln = Ln-1 + Le * Wn; Ln-1= 0
      return sky_color * cur_attenuation;
    }
  }
  // qua 50 lan thi coi nhu bi hap thu
  return Color(0.0f, 0.0f, 0.0f);
}

// Kernel chay tren Gpu khoi tao hat giong ngau nhien cho tung pixel
__global__ void render_init_curand(int width, int height, curandState *rand_state)
{
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  int j = threadIdx.y + blockIdx.y * blockDim.y;
  if (i >= width || j >= height)
    return;
  int pixel_index = j * width + i;
  // Khởi tạo hạt giống (seed) khác nhau cho từng pixel
  curand_init(1984 + pixel_index, 0, 0, &rand_state[pixel_index]);
}

__global__ void create_world_kernel(Hittable **d_list, Hittable **d_world)
{
  if (threadIdx.x == 0 && blockIdx.x == 0)
  {
    // con tro toi ham hit cua cac vat lieu
    d_list[0] = new Sphere(Point3(1.0, 0.0, -1.0), 0.5f,
                           Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5));
    d_list[1] = new Sphere(Point3(0, -100.5f, -1), 100.0f,
                           Material(MaterialType::LAMBERTIAN, Vec3(0.8f, 0.8f, 0.0f), 0, 0));
    d_list[2] = new Sphere(Point3(-1.0, -0.0, -1.0), 0.5f,
                           Material(MaterialType::METAL, Vec3(0.8f, 0.8f, 0.8f), 0.4f, 0));
    d_list[3] = new Sphere(Point3(0.0, 0.0, -1.2), 0.5f,
                           Material(MaterialType::LAMBERTIAN, Vec3(0.2f, 0.5f, 0.8f), 0, 0));
    d_list[4] = new Sphere(Point3(1.0, 0.0, -1.0), -0.4f,
                           Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f));
  }

  // mang dong
  *d_world = new HittableList(d_list, 5);
}

void create_world_wrapper(Hittable **d_list, Hittable **d_world)
{
  create_world_kernel<<<1, 1>>>(d_list, d_world);
  cudaDeviceSynchronize();
}

void allocate_and_init_curand(curandState **d_rand_state, int width, int height)
{
  // Lúc này trình biên dịch nvcc đã biết curandState to chừng nào nên sizeof()
  // chạy ngon lành
  cudaMalloc((void **)d_rand_state, width * height * sizeof(curandState));

  int tx = 8;
  int ty = 8;
  dim3 blocks(width / tx + 1, height / ty + 1);
  dim3 threads(tx, ty);

  // Gọi GPU chạy khởi tạo
  render_init_curand<<<blocks, threads>>>(width, height, *d_rand_state);
  cudaDeviceSynchronize();
}

// Kernel chinh chay tren GP
__global__ void render_anim_kernel(int width, int height, uchar4 *d_output,
                                   Vec3 *d_accumulation_buffer, Hittable **d_world,
                                   curandState *d_rand_state, int frame_count, float time)
{
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  int j = threadIdx.y + blockIdx.y * blockDim.y;

  if (i >= width || j >= height)
    return;

  int pixel_index = j * width + i;

  curandState local_rand_state = d_rand_state[pixel_index];

  // setup camera
  float as = float(width) / float(height);
  float vh = 2.0f;
  float vw = vh * as;
  Point3 origin(0, 0, 0);
  float focus_length = 1.0f;
  Vec3 vvw(vw, 0, 0);
  Vec3 vvh(0, -vh, 0);
  Vec3 vdvw = vvw / width;
  Vec3 vdvh = vvh / height;

  Point3 ul = Vec3(0, 0, -focus_length) - vvw / 2 - vvh / 2 + vdvw / 2 + vdvw / 2;

  float rand_u = curand_uniform(&local_rand_state);
  float rand_v = curand_uniform(&local_rand_state);

  Ray ray_pixel(origin, ul + vdvw * (i + rand_u) + vdvh * (j + rand_v));
  Color pixel_color = ray_color(ray_pixel, d_world, &local_rand_state);

  // Temporal accumulation
  Vec3 accumulated_color;
  if (frame_count == 1)
  {
    accumulated_color = pixel_color;
  }
  else
  {
    accumulated_color = d_accumulation_buffer[pixel_index] + pixel_color;
  }

  d_accumulation_buffer[pixel_index] = accumulated_color;
  Color final_color = accumulated_color / float(frame_count);

  final_color.e[0] = sqrt(final_color.e[0]);
  final_color.e[1] = sqrt(final_color.e[1]);
  final_color.e[2] = sqrt(final_color.e[2]);
  // Ghi mau vao VRAM
  d_output[pixel_index].x = (unsigned char)(255.99f * final_color.x());
  d_output[pixel_index].y = (unsigned char)(255.99f * final_color.y());
  d_output[pixel_index].z = (unsigned char)(255.99f * final_color.z());
  d_output[pixel_index].w = 255;

  d_rand_state[pixel_index] = local_rand_state;
}

// Ham goi tu CPU
void launch_render_kernel(int width, int height, uchar4 *d_output, Vec3 *d_accumlation_buffer,
                          curandState *d_rand_state, int frame_count, Hittable **d_world,
                          float time)
{
  int tx = 8;
  int ty = 8;
  dim3 blocks(width / tx + 1, height / ty + 1);
  dim3 threads(tx, ty);

  render_anim_kernel<<<blocks, threads>>>(width, height, d_output, d_accumlation_buffer, d_world,
                                          d_rand_state, frame_count, time);

  cudaDeviceSynchronize();
}

// free

// Kernel giải phóng các đối tượng đã khởi tạo bằng `new` trong VRAM
__global__ void free_world_kernel(Hittable **d_list, Hittable **d_world)
{
  if (threadIdx.x == 0 && blockIdx.x == 0)
  {
    delete d_list[0]; // Xóa quả cầu chính
    delete d_list[1]; // Xóa quả cầu mặt đất khổng lồ
    delete d_list[2];
    delete d_list[3];
    delete *d_world; // Xóa chiếc Rổ (HittableList)
  }
}

// Hàm Wrapper để main.cpp có thể gọi được kernel ở trên
void free_world_wrapper(Hittable **d_list, Hittable **d_world)
{
  free_world_kernel<<<1, 1>>>(d_list, d_world);
  cudaDeviceSynchronize();
}
