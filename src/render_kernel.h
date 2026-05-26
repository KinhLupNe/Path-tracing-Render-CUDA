#pragma once

// File vector_types.h là thư viện lõi của CUDA. 
// Nó chứa sẵn cấu trúc uchar4 (Một cấu trúc gồm 4 biến x, y, z, w kiểu byte - tương đương Red, Green, Blue, Alpha).
#include <vector_types.h>
#include "math/vec3.h"

// Khai báo nguyên mẫu (Prototype) của hàm wrapper.
// Hàm này được viết chi tiết ở render_kernel.cu, nhưng khai báo ở đây để main.cpp (CPU) có thể nhìn thấy và gọi nó.
// Các tham số: chiều rộng, chiều cao, con trỏ VRAM chứa ảnh (d_output), tâm quả cầu và bán kính.
void launch_render_kernel(int width, int height, uchar4* d_output, Point3 sphere_center, float sphere_radius);
