#include "math/hittable/sphere.h"
#include "math/ray.h"
#include "math/vec3.h"
#include "render_kernel.h"
// tinh mau cho 1 pixel
__device__ Color ray_color(const Ray &r, Point3 sphere_center, float sphere_radius) {
  Sphere s(sphere_center, sphere_radius);
  hit_record rec;

  if (s.hit(r, 0.0f, 100.0f, rec)) {
    // Trung -> Tra ve mau theo phap tuyen (Normal)
    return 0.5f * Color(rec.normal.x() + 1.0f, rec.normal.y() + 1.0f,
                        rec.normal.z() + 1.0f);
  }

  // Truot -> Ve mau bau troi
  Vec3 unit_direction = unit_vector(r.direction());
  float t = 0.5 * (unit_direction.y() + 1.0f);
  return (1.0f - t) * Color(1.0f, 1.0f, 1.0f) + t * Color(0.5f, 0.2f, 1.0f);
}

// Kernel chinh chay tren GPU
__global__ void render_anim_kernel(int width, int height, uchar4 *d_output, Point3 sphere_center, float sphere_radius) {
  int i = threadIdx.x + blockIdx.x * blockDim.x;
  int j = threadIdx.y + blockIdx.y * blockDim.y;

  if (i >= width || j >= height)
    return;

  // Setup Camera
  float aspect_ratio = float(width) / float(height);
  float viewport_height = 2.0f;
  float viewport_width = aspect_ratio * viewport_height;
  float focal_length = 1.0f;
  Point3 origin = Point3(0, 0, 0);
  Vec3 horizontal = Vec3(viewport_width, 0, 0);
  Vec3 vertical = Vec3(0, viewport_height, 0);
  Point3 lower_left_corner =
      origin - horizontal / 2.0f - vertical / 2.0f - Vec3(0, 0, focal_length);

  // Mapping Pixel -> Viewport
  float u = float(i) / float(width - 1);
  float v = float(height - 1 - j) / float(height - 1);

  // Ban tia sang
  Ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
  Color pixel_color = ray_color(r, sphere_center, sphere_radius);

  // Ghi mau vao VRAM
  int pixel_index = j * width + i;
  d_output[pixel_index].x = (unsigned char)(255.99f * pixel_color.x());
  d_output[pixel_index].y = (unsigned char)(255.99f * pixel_color.y());
  d_output[pixel_index].z = (unsigned char)(255.99f * pixel_color.z());
  d_output[pixel_index].w = 255;
}

// Ham goi tu CPU
void launch_render_kernel(int width, int height, uchar4 *d_output, Point3 sphere_center, float sphere_radius) {
  int tx = 8;
  int ty = 8;
  dim3 blocks(width / tx + 1, height / ty + 1);
  dim3 threads(tx, ty);

  render_anim_kernel<<<blocks, threads>>>(width, height, d_output, sphere_center, sphere_radius);
  cudaDeviceSynchronize();
}
