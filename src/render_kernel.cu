#include "geometry/hittable.h"
#include "geometry/sphere.h"
#include "geometry/moving_sphere.h"
#include "material/material.h"
#include "math/ray.h"
#include "math/vec3.h"
#include "core/scene.h"
#include "render_kernel.h"

#include <cmath>
#include <cuda_runtime_api.h>
#include <curand_kernel.h>
#include <curand_uniform.h>
#include "geometry/hittable_list.h"

// schlick appro -> reflect coef -> he so nay dai dien cho ti le tia phan xa/ tia toi
__device__ inline float reflectionSchlickA(float cosine, float ir) {
  float r = (1 - ir) / (1 + ir);
  r = r * r;
  return r + (1 - r) * powf(1 - cosine, 5);
}
// input la tia vao, ouput : tan xa hay khong ? co tra ve scattered attenuation
__device__ bool scatter(const Ray &r_in, const hit_record &rec, Vec3 &attenuation, Ray &scattered,
                        curandState *local_rand_state) {
  switch (rec.mat.type) {
  case MaterialType::LAMBERTIAN: {
    Vec3 scatter_direction = rec.normal + random_unit_vector(local_rand_state);
    scattered = Ray(rec.p, scatter_direction, r_in.time());
    attenuation = rec.mat.albedo;
    return (dot(scattered.direction(), rec.normal) > 0.0f);
  }
  case MaterialType::METAL: {
    Vec3 reflected = reflect(r_in.direction(), rec.normal);

    // fuzz: kim loai nham
    scattered = Ray(rec.p, reflected + random_in_unit_vector(local_rand_state) * rec.mat.fuzz, r_in.time());
    attenuation = rec.mat.albedo;
    // se co tia sang dam vao trong vat the, chi tiep tuc khi phan ra ngoai
    return (dot(scattered.direction(), rec.normal) > 0.0f);
  }
  case MaterialType::DIELECTRIC: {
    attenuation = rec.mat.albedo;
    // di từ môi trường vào quả cầu , hay cầu đi ra ngoài môi trường
    float a = dot(rec.normal, r_in.direction()) < 0 ? (1.0f / rec.mat.ir) : rec.mat.ir;
    Vec3 out_normal = a == rec.mat.ir ? -rec.normal : rec.normal;
    float costheta = fabs(dot(rec.normal, unit_vector(r_in.direction())));
    bool isRefract = a * sqrtf(1.0f - costheta * costheta) > 1.0f ? false : true;
    Vec3 scatter_direction;
    if (!isRefract ||
        reflectionSchlickA(costheta, rec.mat.ir) > random_float(0.0f, 1.0f, local_rand_state)) {
      scatter_direction = reflect(r_in.direction(), out_normal);
    } else {
      scatter_direction = refract(unit_vector(r_in.direction()), a, out_normal);
    }
    scattered = Ray(rec.p, unit_vector(scatter_direction), r_in.time());
    return true;
  }
  default:
    return false;
  }
}
// tinh mau cho 1 pixel// path tracing
__device__ Color ray_color(const Ray &r, Hittable **world, curandState *local_rand_state) {
  Ray cur_ray = r;
  Color cur_attenuation(1.0f, 1.0f, 1.0f);

  for (int i = 0; i < 50; i++) {
    hit_record rec;
    if ((*world)->hit(cur_ray, 0.001f, 10000.0f, rec)) {
      Ray scattered;
      Vec3 attenuation;
      if (scatter(cur_ray, rec, attenuation, scattered, local_rand_state)) {
        // Wn = Beta *Wn-1
        cur_attenuation = cur_attenuation * attenuation;
        // Tối ưu hóa: Nếu tia sáng bị hấp thụ hoàn toàn (đập vào vật màu đen), dừng lại luôn để đỡ tốn GPU
        if (cur_attenuation.x() < 0.001f && cur_attenuation.y() < 0.001f && cur_attenuation.z() < 0.001f) {
            return Color(0.0f, 0.0f, 0.0f);
        }
        cur_ray = scattered;
      } else {
        // khong tan xa thi cook
        return Color(0.0f, 0.0f, 0.0f);
      }
    } else {
      Vec3 unit_direction = unit_vector(cur_ray.direction());
      float a = 0.5f * (unit_direction.y() + 1.0f);
      Color sky_color = (1.0f - a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 1.0f);
      // Ln = Ln-1 + Le * Wn; Ln-1= 0
      return sky_color * cur_attenuation;
    }
  }
  // qua 50 lan thi coi nhu bi hap thu
  return Color(0.0f, 0.0f, 0.0f);
}

// Kernel chay tren Gpu khoi tao hat giong ngau nhien cho tung pixel
__global__ void render_init_curand(int width, int height, curandState *rand_state) {
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  int j = threadIdx.y + blockIdx.y * blockDim.y;
  if (i >= width || j >= height)
    return;
  int pixel_index = j * width + i;
  // Khởi tạo hạt giống (seed) khác nhau cho từng pixel
  curand_init(1984 + pixel_index, 0, 0, &rand_state[pixel_index]);
}

// So luong object thuc te trong scene (set luc tao world, doc lai luc free).
__device__ int d_world_size = 0;

__global__ void create_world_kernel(Hittable **d_list, Hittable **d_world, Camera **d_camera,
                                    SphereData *d_spheres, int num_spheres, const Camera host_camera) {
  if (threadIdx.x == 0 && blockIdx.x == 0) {
    // Tao danh sach cac doi tuong tu mang SphereData
    for (int i = 0; i < num_spheres; i++) {
        if (d_spheres[i].is_moving) {
            d_list[i] = new MovingSphere(d_spheres[i].center, d_spheres[i].center2, 
                                         d_spheres[i].time0, d_spheres[i].time1, 
                                         d_spheres[i].radius, d_spheres[i].mat);
        } else {
            d_list[i] = new Sphere(d_spheres[i].center, d_spheres[i].radius, d_spheres[i].mat);
        }
    }
    *d_world = new HittableList(d_list, num_spheres);
    d_world_size = num_spheres;

    // Dùng camera được pass từ CPU xuống (đã có time0, time1)
    *d_camera = new Camera(host_camera);
  }
}

void create_world_wrapper(Hittable **d_list, Hittable **d_world, Camera **d_camera, SphereData *d_spheres, int num_spheres, const Camera& host_camera) {
  // Khởi tạo các Sphere và Camera bằng Kernel
  int threads = 256;
  int blocks = (num_spheres + threads - 1) / threads;
  if (num_spheres > 0) {
    create_world_kernel<<<blocks, threads>>>(d_list, d_world, d_camera, d_spheres, num_spheres, host_camera);
  }
  cudaDeviceSynchronize();
}

__global__ void update_camera_kernel(Camera **d_camera, Camera host_camera) {
  if (threadIdx.x == 0 && blockIdx.x == 0) {
    **d_camera = host_camera;
  }
}

void update_camera_wrapper(Camera **d_camera, const Camera& host_camera) {
  update_camera_kernel<<<1, 1>>>(d_camera, host_camera);
  cudaDeviceSynchronize();
}

void allocate_and_init_curand(curandState **d_rand_state, int width, int height) {
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
                                   Vec3 *d_accumulation_buffer, Hittable **d_world, Camera **d_camera,
                                   curandState *d_rand_state, int frame_count, float time) {
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  int j = threadIdx.y + blockIdx.y * blockDim.y;

  if (i >= width || j >= height)
    return;

  int pixel_index = j * width + i;

  curandState local_rand_state = d_rand_state[pixel_index];

  float rand_u = curand_uniform(&local_rand_state);
  float rand_v = curand_uniform(&local_rand_state);

  float s = float(i + rand_u) / float(width);
  float t = 1.0f - float(j + rand_v) / float(height);

  Ray ray_pixel = (*d_camera)->get_ray(s, t, &local_rand_state);
  Color pixel_color = ray_color(ray_pixel, d_world, &local_rand_state);

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
void launch_render_kernel(int width, int height, uchar4 *d_output, Vec3 *d_accumulation_buffer,
                          curandState *d_rand_state, int frame_count, Hittable **d_world, Camera **d_camera,
                          float time) {
  int tx = 8;
  int ty = 8;
  dim3 blocks(width / tx + 1, height / ty + 1);
  dim3 threads(tx, ty);

  render_anim_kernel<<<blocks, threads>>>(width, height, d_output, d_accumulation_buffer, d_world, d_camera,
                                          d_rand_state, frame_count, time);

  cudaDeviceSynchronize();
}

// free

// Kernel giải phóng các đối tượng đã khởi tạo bằng `new` trong VRAM
__global__ void free_world_kernel(Hittable **d_list, Hittable **d_world, Camera **d_camera) {
  if (threadIdx.x == 0 && blockIdx.x == 0) {
    // Xoa tat ca object da tao (so luong luu trong d_world_size)
    for (int i = 0; i < d_world_size; i++) {
      delete d_list[i];
    }
    delete *d_world; // Xóa chiếc Rổ (HittableList)
    delete *d_camera;
  }
}

// Hàm Wrapper để main.cpp có thể gọi được kernel ở trên
void free_world_wrapper(Hittable **d_list, Hittable **d_world, Camera **d_camera) {
  free_world_kernel<<<1, 1>>>(d_list, d_world, d_camera);
  cudaDeviceSynchronize();
}
