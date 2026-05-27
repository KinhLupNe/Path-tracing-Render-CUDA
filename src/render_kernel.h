#pragma once
#include "curand_kernel.h"
#include "math/vec3.h"
#include <vector_types.h>

struct curandStateXORWOW;
typedef struct curandStateXORWOW curandState;

void allocate_and_init_curand(curandState **d_rand_state, int width,
                              int height);
void launch_render_kernel(int width, int height, uchar4 *d_output,
                          Vec3 *d_accumulation_buffer,
                          curandState *d_rand_state, int frame_count,
                          Point3 sphere_center, float sphere_radius,
                          float time);
