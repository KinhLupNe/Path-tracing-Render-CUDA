#include "math/hittable/sphere.h"
#include "math/ray.h"
#include "math/vec3.h"
#include "render_kernel.h"
#include <cmath>
#include <curand_kernel.h>
#include <curand_uniform.h>
// tinh mau cho 1 pixel
__device__ Color ray_color(const Ray &r, Point3 sphere_center,
                           float sphere_radius, float time) {
  Sphere s(sphere_center + Vec3(0, sinf(time) / 2, 0), sphere_radius);
  hit_record rec;
  if (s.hit(r, 0.0f, 100.0f, rec)) {
    // Normal shading: ánh xạ pháp tuyến [-1, 1] sang màu RGB [0, 1]
    return 0.5f * Color(rec.normal.x() + 1.0f, rec.normal.y() + 1.0f,
                        rec.normal.z() + 1.0f);
  }

  // Bầu trời (Skybox gradient)
  Vec3 unit_direction = unit_vector(r.direction());
  float a = 0.5f * (unit_direction.y() + 1.0f);
  return (1.0f - a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 1.0f);
}

// Kernel chay tren Gpu khoi tao hat giong ngau nhien cho tung pixel
__global__ void render_init_curand(int width, int height,
                                   curandState *rand_state) {
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  int j = threadIdx.y + blockIdx.y * blockDim.y;
  if (i >= width || j >= height)
    return;
  int pixel_index = j * width + i;
  // Khởi tạo hạt giống (seed) khác nhau cho từng pixel
  curand_init(1984 + pixel_index, 0, 0, &rand_state[pixel_index]);
}

// 2. Hàm Wrapper (chạy trên CPU) giúp main.cpp cấp phát VRAM và gọi Kernel ở
// trên
void allocate_and_init_curand(curandState **d_rand_state, int width,
                              int height) {
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
                                   Vec3 *d_accumulation_buffer,
                                   curandState *d_rand_state, int frame_count,
                                   Point3 sphere_center, float sphere_radius,
                                   float time) {
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

  Point3 ul =
      Vec3(0, 0, -focus_length) - vvw / 2 - vvh / 2 + vdvw / 2 + vdvw / 2;

  float rand_u = curand_uniform(&local_rand_state);
  float rand_v = curand_uniform(&local_rand_state);

  Ray ray_pixel(origin, ul + vdvw * (i + rand_u) + vdvh * (j + rand_v));
  Color pixel_color = ray_color(ray_pixel, sphere_center, sphere_radius, time);

  // Temporal accumulation
  Vec3 accumulated_color;
  if (frame_count == 1) {
    accumulated_color = pixel_color;
  } else {
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
void launch_render_kernel(int width, int height, uchar4 *d_output,
                          Vec3 *d_accumlation_buffer, curandState *d_rand_state,
                          int frame_count, Point3 sphere_center,
                          float sphere_radius, float time) {
  int tx = 8;
  int ty = 8;
  dim3 blocks(width / tx + 1, height / ty + 1);
  dim3 threads(tx, ty);

  render_anim_kernel<<<blocks, threads>>>(
      width, height, d_output, d_accumlation_buffer, d_rand_state, frame_count,
      sphere_center, sphere_radius, time);

  cudaDeviceSynchronize();
}
