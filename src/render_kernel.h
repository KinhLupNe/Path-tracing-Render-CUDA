#pragma once
#include "curand_kernel.h"
#include "math/vec3.h"
#include <vector_types.h>
#include "geometry/hittable.h"
#include "core/camera.h"
#include "core/scene.h"

// So luong toi da object trong scene (suc chua cua d_list).
// Scene phuc tap sinh ngau nhien nen chi can dam bao d_list du lon;
// so luong thuc te se duoc luu trong bien global d_world_size.
#define MAX_HITTABLES 10000

struct curandStateXORWOW;
typedef struct curandStateXORWOW curandState;

void allocate_and_init_curand(curandState **d_rand_state, int width, int height);
void launch_render_kernel(int width, int height, uchar4 *d_output, Vec3 *d_accumulation_buffer,
                          curandState *d_rand_state, int frame_count, Hittable **d_world, Camera **d_camera,
                          float time, float vignette_strength);

void create_world_wrapper(Hittable **d_list, Hittable **d_world, Camera **d_camera, SphereData *d_spheres, int num_spheres, const Camera& host_camera);

// void update_world_wrapper(Hittable **d_list, Point3 base_center, float time);
void free_world_wrapper(Hittable **d_list, Hittable **d_world, Camera **d_camera);

void update_camera_wrapper(Camera **d_camera, const Camera& host_camera);
