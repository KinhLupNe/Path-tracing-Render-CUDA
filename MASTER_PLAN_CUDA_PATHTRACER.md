# CUỐN CẨM NANG TOÀN TẬP: TỪ LÝ THUYẾT PBRT ĐẾN THỰC TIỄN CUDA PATH TRACER
**Mục tiêu phần cứng:** NVIDIA GTX 1650 (Kiến trúc Turing TU117, 896 CUDA Cores, 4GB VRAM, Không có RT Cores).

---

## MỤC LỤC
1. [Chương 1: Triết Lý Thiết Kế - Tại Sao PBRT OOP Không Hợp Với GPU](#chuong-1)
2. [Chương 2: Cấu Trúc Dự Án & Hệ Thống Build (CMake)](#chuong-2)
3. [Chương 3: Cấu Trúc Toán Học & Quản Lý Cấu Hình (Math & Utils)](#chuong-3)
4. [Chương 4: Quản Lý Bộ Nhớ & Tích Hợp OpenGL (Display)](#chuong-4)
5. [Chương 5: Hình Học & Bài Toán Giao Cắt (Geometry & Intersections)](#chuong-5)
6. [Chương 6: Cấu Trúc Gia Tốc Không Gian (BVH - Bounding Volume Hierarchy)](#chuong-6)
7. [Chương 7: Vật Liệu (Materials) & Tán Xạ (Scattering)](#chuong-7)
8. [Chương 8: Cốt Lõi Path Tracing (The Megakernel)](#chuong-8)
9. [Chương 9: Các Kỹ Thuật Tối Ưu Nâng Cao Cho GTX 1650](#chuong-9)

---

<a id="chuong-1"></a>
## CHƯƠNG 1: TRIẾT LÝ THIẾT KẾ - TẠI SAO PBRT OOP KHÔNG HỢP VỚI GPU

PBRT v4 là một kiệt tác về kỹ thuật phần mềm hướng đối tượng (OOP). Nó trừu tượng hóa mọi thứ: `Shape` là class ảo, `Material` là class ảo, `BxDF` là class ảo. Khi bắn một tia, PBRT gọi `shape->Intersect()`. 

**Tại sao điều này là "án tử" trên GPU GTX 1650?**

1. **Vtable & Virtual Function Call Overhead:** Trên C++, khi gọi hàm ảo, CPU phải tra cứu bảng vtable. Mất thêm chu kỳ nhịp. Trên GPU, việc mỗi thread trong 1 Warp (32 threads) phải tra cứu địa chỉ hàm ảo khác nhau sẽ phá vỡ tính song song.
2. **Warp Divergence (Rẽ nhánh Warp):** Nếu tia 1 cắt hình cầu (Sphere), tia 2 cắt tam giác (Triangle), các thread sẽ phải chạy hai đoạn code khác nhau. GPU xử lý việc này bằng cách: Chạy code của Sphere trong khi các thread Triangle ĐỨNG ĐỢI, sau đó ngược lại. Hiệu năng giảm một nửa.
3. **Memory Layout (AoS vs SoA):** OOP thường gom dữ liệu thành Array of Structures (AoS - ví dụ mảng các Object). GPU lại thích Structure of Arrays (SoA) hoặc Flattened Arrays để có thể đọc bộ nhớ liên tục (Memory Coalescing).

**GIẢI PHÁP: DATA-ORIENTED DESIGN (DOD)**

Chúng ta sẽ đập bỏ hoàn toàn OOP. Mọi thứ trên GPU sẽ là struct dữ liệu thuần túy (Plain Old Data - POD).
- Không có kế thừa (Inheritance).
- Không có hàm ảo (Virtual functions).
- Sử dụng thẻ định danh (Tag/Enum) để phân biệt loại vật liệu hoặc hình học.

---

<a id="chuong-2"></a>
## CHƯƠNG 2: CẤU TRÚC DỰ ÁN & HỆ THỐNG BUILD (CMAKE)

### 2.1. Cấu trúc thư mục chuẩn
```text
ScratchPathTracerCUDA/
├── CMakeLists.txt          (File cấu hình build)
├── src/
│   ├── main.cpp            (Điểm vào của chương trình, tạo window OpenGL)
│   ├── render_kernel.cu    (File chứa code chạy trên GPU - CUDA)
│   ├── render_kernel.h     (Header định nghĩa hàm gọi kernel)
│   ├── math_utils.h        (Thư viện Vec3, Ray)
│   ├── scene_data.h        (Struct định nghĩa Material, Triangle, BVHNode)
│   └── bvh_builder.cpp     (Chạy trên CPU để xây BVH trước khi đẩy lên GPU)
├── third_party/
│   ├── glfw/               (Tạo cửa sổ)
│   ├── glad/               (Load hàm OpenGL)
│   ├── stb_image_write.h   (Ghi ảnh ra file)
│   └── tiny_obj_loader.h   (Load file mô hình 3D .obj)
└── models/                 (Chứa file .obj)
```

### 2.2. File `CMakeLists.txt` cực kỳ chi tiết
Đây là file bạn sẽ cần để gộp chung mã C++ và CUDA:

```cmake
cmake_minimum_required(VERSION 3.18)
project(ScratchPathTracerCUDA LANGUAGES CXX CUDA)

# Cấu hình chuẩn C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tối ưu hóa mạnh nhất cho CUDA
# -O3: Bật mọi tối ưu hóa.
# -use_fast_math: Sinh mã toán học siêu nhanh (chấp nhận sai số nhỏ cực kỳ hợp với đồ họa).
# arch=sm_75: Kiến trúc Turing của GTX 1650.
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3 -use_fast_math -arch=sm_75")

# Khai báo file nguồn
set(SOURCES
    src/main.cpp
    src/bvh_builder.cpp
    src/render_kernel.cu
)

# Tạo executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories (Thêm thư mục gốc vào đường dẫn)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Tích hợp thư viện đồ họa (Giả định bạn đã cài OpenGL và GLFW)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
)
```

---

<a id="chuong-3"></a>
## CHƯƠNG 3: CẤU TRÚC TOÁN HỌC & QUẢN LÝ CẤU HÌNH

Chúng ta không sử dụng Eigen hay GLM vì chúng quá cồng kềnh cho CUDA. Hãy viết struct `Vec3` riêng biệt, phải có từ khóa `__host__ __device__` để có thể dùng trên cả CPU và GPU.

```cpp
// src/math_utils.h
#pragma once
#include <cuda_runtime.h>
#include <math.h>

struct Vec3 {
    float x, y, z;
    
    __host__ __device__ Vec3() : x(0), y(0), z(0) {}
    __host__ __device__ Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    // Các toán tử thiết yếu
    __host__ __device__ inline Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    __host__ __device__ inline Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    __host__ __device__ inline Vec3 operator*(float t) const { return Vec3(x*t, y*t, z*t); }
    __host__ __device__ inline Vec3 operator*(const Vec3& v) const { return Vec3(x*v.x, y*v.y, z*v.z); } // Nhân từng thành phần (Color)
};

// Dot product
__host__ __device__ inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// Cross product
__host__ __device__ inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

__host__ __device__ inline Vec3 normalize(const Vec3& v) {
    float invLen = rsqrtf(v.x*v.x + v.y*v.y + v.z*v.z); // Lệnh asm siêu nhanh của CUDA
    return v * invLen;
}

// Cấu trúc Tia sáng
struct Ray {
    Vec3 orig;
    Vec3 dir;
    __host__ __device__ Ray() {}
    __host__ __device__ Ray(const Vec3& origin, const Vec3& direction) : orig(origin), dir(direction) {}
    __host__ __device__ Vec3 at(float t) const { return orig + dir * t; }
};
```

---

<a id="chuong-4"></a>
## CHƯƠNG 4: QUẢN LÝ BỘ NHỚ & HIỂN THỊ (DISPLAY)

Bạn cần phân bổ bộ nhớ trên GTX 1650 thật sự cẩn thận.
Chúng ta sẽ có một Accumulation Buffer (Kiểu float) để cộng dồn màu qua các lần mẫu (samples). Và một Display Buffer (Kiểu byte) để vẽ lên màn hình.

### 4.1. Khởi tạo bộ nhớ (Host gọi tới Device)
```cpp
// Cấp phát Accumulation Buffer (float3)
float3* d_accumulation_buffer;
cudaMalloc((void**)&d_accumulation_buffer, width * height * sizeof(float3));

// Cấp phát bộ nhớ cho trạng thái cuRAND
curandState* d_rand_states;
cudaMalloc((void**)&d_rand_states, width * height * sizeof(curandState));

// Xóa buffer
cudaMemset(d_accumulation_buffer, 0, width * height * sizeof(float3));
```

### 4.2. Khởi tạo ngẫu nhiên (cuRAND Init Kernel)
Mỗi pixel (thread) phải có một mầm (seed) ngẫu nhiên riêng để các tia không bị bắn giống hệt nhau (gây ra pattern nhiễu).
```cuda
#include <curand_kernel.h>

__global__ void render_init(int max_x, int max_y, curandState *rand_state) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    int j = threadIdx.y + blockIdx.y * blockDim.y;
    if((i >= max_x) || (j >= max_y)) return;
    int pixel_index = j*max_x + i;
    
    // Seed được tính toán kết hợp tọa độ pixel để đảm bảo độc lập
    curand_init(1984 + pixel_index, 0, 0, &rand_state[pixel_index]);
}
```

---

<a id="chuong-5"></a>
## CHƯƠNG 5: HÌNH HỌC VÀ BÀI TOÁN GIAO CẮT

Chúng ta sẽ từ bỏ OOP. Mọi thứ trong scene sẽ chỉ có 1 loại mảng hình học cơ bản: **Tam giác (Triangle)**. Các hình cầu (nếu có) sẽ được định nghĩa là một Struct tách biệt và xử lý trên mảng riêng. Tuy nhiên, để làm một renderer tổng quát load mô hình `.obj`, dùng duy nhất Triangle là lựa chọn khôn ngoan nhất cho GPU.

```cpp
struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 n0, n1, n2; // Pháp tuyến đỉnh (để tính smooth shading)
    int material_id; // Chỉ mục trỏ tới mảng Material
};
```

### Thuật toán cắt tam giác cực tốc: Möller–Trumbore
Đây là chuẩn mực công nghiệp vì nó không cần tính phương trình mặt phẳng, giảm áp lực bộ nhớ.

```cuda
__device__ bool hit_triangle(const Ray& r, const Triangle& tri, float t_min, float t_max, float& t_out, Vec3& normal_out) {
    Vec3 edge1 = tri.v1 - tri.v0;
    Vec3 edge2 = tri.v2 - tri.v0;
    Vec3 h = cross(r.dir, edge2);
    float a = dot(edge1, h);

    if (a > -1e-8f && a < 1e-8f) return false; // Tia song song mặt phẳng
    float f = 1.0f / a;
    Vec3 s = r.orig - tri.v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    Vec3 q = cross(s, edge1);
    float v = f * dot(r.dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * dot(edge2, q);
    if (t > t_min && t < t_max) {
        t_out = t;
        // Căn bản: Nội suy pháp tuyến đỉnh (Barycentric coordinates)
        normal_out = normalize(tri.n0 * (1 - u - v) + tri.n1 * u + tri.n2 * v);
        return true;
    }
    return false;
}
```

---

<a id="chuong-6"></a>
## CHƯƠNG 6: CẤU TRÚC GIA TỐC KHÔNG GIAN (BVH)

Nếu không có BVH, vòng lặp for kiểm tra hàng vạn tam giác sẽ đánh gục GTX 1650.
Cây BVH (Bounding Volume Hierarchy) phân loại không gian thành các hộp chứa (AABB - Axis Aligned Bounding Box).

### 6.1. Thiết kế Data-Oriented BVH Node (Cực gọn nhẹ)
Kích thước Node là cực kỳ quan trọng. Phải fit vừa vặn thành 32-byte (Kích thước cache line của GPU).

```cpp
struct BVHNode {
    Vec3 aabb_min;      // 12 bytes
    int left_child_idx; // 4 bytes. Nếu là lá, chứa chỉ mục tam giác bắt đầu.
    Vec3 aabb_max;      // 12 bytes
    int triangle_count; // 4 bytes. Nếu > 0, đây là node lá. (Tổng = 32 bytes! Hoàn hảo)
};
```
*Lưu ý:* Mảng `BVHNode` sẽ được nạp lên CUDA Global Memory bằng `cudaMemcpy`.

### 6.2. Duyệt BVH KHÔNG dùng đệ quy trên GPU
GPU đệ quy rất kém (tràn stack). Ta dùng mảng cục bộ đóng vai trò làm Stack (được lưu trên Register hoặc L1 cache tốc độ cao).

```cuda
__device__ bool traverse_bvh(const Ray& r, BVHNode* bvh_nodes, Triangle* triangles, int root_idx, float t_max, int& hit_tri_idx, float& hit_t, Vec3& hit_normal) {
    int stack[64]; // Độ sâu cây tối đa (GPU Registers)
    int stack_ptr = 0;
    stack[stack_ptr++] = root_idx;

    bool hit_anything = false;
    float closest_so_far = t_max;

    while (stack_ptr > 0) {
        int node_idx = stack[--stack_ptr]; // Pop từ stack
        BVHNode& node = bvh_nodes[node_idx];

        // 1. Kiểm tra tia có đâm trúng hộp AABB của node này không?
        if (!hit_aabb(r, node.aabb_min, node.aabb_max, closest_so_far)) continue;

        // 2. Nếu là node lá (có chứa tam giác)
        if (node.triangle_count > 0) {
            for (int i = 0; i < node.triangle_count; ++i) {
                int tri_idx = node.left_child_idx + i;
                float t;
                Vec3 n;
                if (hit_triangle(r, triangles[tri_idx], 0.001f, closest_so_far, t, n)) {
                    hit_anything = true;
                    closest_so_far = t;
                    hit_tri_idx = tri_idx;
                    hit_t = t;
                    hit_normal = n;
                }
            }
        } else {
            // Đẩy child nodes vào stack. (Nên có logic tối ưu: đẩy node gần hơn vào sau để check trước)
            stack[stack_ptr++] = node.left_child_idx + 1; // Right child
            stack[stack_ptr++] = node.left_child_idx;     // Left child
        }
    }
    return hit_anything;
}
```

---

<a id="chuong-7"></a>
## CHƯƠNG 7: VẬT LIỆU (MATERIALS) VÀ TÁN XẠ (SCATTERING)

Flatten toàn bộ BxDF phức tạp của PBRT thành 1 struct Material duy nhất, điều khiển bởi biến `type`.

```cpp
enum MaterialType { LAMBERTIAN = 0, METAL = 1, DIELECTRIC = 2, LIGHT = 3 };

struct Material {
    MaterialType type;
    Vec3 albedo;    // Màu cơ bản hoặc Hệ số hấp thụ
    float roughness; // Độ nhám (Dùng cho Metal/Microfacet)
    float ior;       // Index of Refraction (Dùng cho Dielectric)
    Vec3 emission;   // Dùng cho đèn phát sáng
};
```

**Hàm Scattering (Giải phương trình BRDF):**
Thay vì gọi `material->scatter()`, kernel sẽ có 1 khối switch khổng lồ. Việc này dù tạo ra một chút warp divergence, nhưng vẫn nhẹ hơn overhead gọi hàm ảo rất nhiều.

```cuda
__device__ bool scatter(const Ray& r_in, const Vec3& hit_point, const Vec3& normal, const Material& mat, curandState* local_rand_state, Vec3& attenuation, Ray& scattered) {
    if (mat.type == LAMBERTIAN) {
        // Tán xạ khuếch tán: Bắn tia ngẫu nhiên theo hình bán cầu phân bổ Cosine
        Vec3 target = hit_point + normal + random_in_unit_sphere(local_rand_state);
        scattered = Ray(hit_point, normalize(target - hit_point));
        attenuation = mat.albedo;
        return true;
    } 
    else if (mat.type == METAL) {
        // Phản xạ gương (Mirror) có độ nhám
        Vec3 reflected = reflect(r_in.dir, normal);
        scattered = Ray(hit_point, reflected + mat.roughness * random_in_unit_sphere(local_rand_state));
        attenuation = mat.albedo;
        return (dot(scattered.dir, normal) > 0);
    }
    //... DIELECTRIC khúc xạ sử dụng Fresnel Schlick ...
    return false;
}
```

---

<a id="chuong-8"></a>
## CHƯƠNG 8: KERNEL CỐT LÕI (THE PATH TRACING MEGAKERNEL)

Bây giờ là thời khắc kết nối tất cả lại. Trái tim của chương trình. Chúng ta sử dụng vòng lặp `for` hoặc `while` để thay thế đệ quy `Li()` trong PBRT.

```cuda
__global__ void render_kernel(
    int width, int height, int samples_per_launch, int max_depth,
    float3* accum_buffer,
    curandState* rand_states,
    Camera camera,
    BVHNode* bvh_nodes, Triangle* triangles, Material* materials) 
{
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    int j = threadIdx.y + blockIdx.y * blockDim.y;
    if (i >= width || j >= height) return;
    int pixel_index = j * width + i;

    curandState local_rand_state = rand_states[pixel_index];
    Vec3 pixel_color(0, 0, 0);

    for (int s = 0; s < samples_per_launch; ++s) {
        // Chống răng cưa (Anti-aliasing) bằng cách rung tọa độ tia gốc
        float u = float(i + curand_uniform(&local_rand_state)) / float(width);
        float v = float(j + curand_uniform(&local_rand_state)) / float(height);
        
        Ray r = camera.get_ray(u, v);
        
        Vec3 cur_attenuation(1.0f, 1.0f, 1.0f); // Thông lượng sáng (Throughput)
        Vec3 sample_color(0.0f, 0.0f, 0.0f);

        // Vòng lặp Path Tracing cốt lõi
        for (int depth = 0; depth < max_depth; ++depth) {
            int hit_tri_idx = -1;
            float hit_t;
            Vec3 hit_normal;

            if (traverse_bvh(r, bvh_nodes, triangles, 0, 99999.0f, hit_tri_idx, hit_t, hit_normal)) {
                Vec3 hit_point = r.at(hit_t);
                Material mat = materials[triangles[hit_tri_idx].material_id];

                // Nếu là đèn, phát sáng rồi kết thúc
                if (mat.type == LIGHT) {
                    sample_color = sample_color + (cur_attenuation * mat.emission);
                    break;
                }

                Ray scattered;
                Vec3 attenuation;
                if (scatter(r, hit_point, hit_normal, mat, &local_rand_state, attenuation, scattered)) {
                    cur_attenuation = cur_attenuation * attenuation; // Nhân thông lượng qua từng lần nảy
                    r = scattered; // Cập nhật tia mới
                } else {
                    break; // Bị hấp thụ hoàn toàn
                }
            } else {
                // Đâm lên trời (Skybox)
                Vec3 unit_direction = normalize(r.dir);
                float t = 0.5f * (unit_direction.y + 1.0f);
                Vec3 skybox = Vec3(1.0, 1.0, 1.0) * (1.0f - t) + Vec3(0.5, 0.7, 1.0) * t;
                sample_color = sample_color + (cur_attenuation * skybox);
                break;
            }
        }
        pixel_color = pixel_color + sample_color;
    }

    // Ghi vào Accumulation Buffer
    accum_buffer[pixel_index] = accum_buffer[pixel_index] + pixel_color;
    
    // Lưu lại trạng thái rand (Cực kỳ quan trọng để frame sau random tiếp)
    rand_states[pixel_index] = local_rand_state;
}
```

---

<a id="chuong-9"></a>
## CHƯƠNG 9: CÁC KỸ THUẬT TỐI ƯU NÂNG CAO CHO GTX 1650

Megakernel (dồn tất cả logic vào 1 kernel như trên) là cách dễ nhất để học, nhưng sẽ vấp phải giới hạn của phần cứng. Để "vắt kiệt" 896 nhân CUDA của con GTX 1650:

### 1. Register Pressure (Áp lực thanh ghi)
Cái `while` loop của BVH traverse sử dụng mảng stack `int stack[64];`. Đây là một quả bom nổ chậm.
Nếu GPU không đủ Registers để cấp phát cho mỗi Thread, nó sẽ đổ dữ liệu đó ra *Local Memory* (cực chậm vì Local Memory thực chất nằm trên Global DRAM).
**Giải pháp:** 
- Đặt compiler flag: `-maxrregcount=64` (Ép biên dịch chỉ dùng tối đa 64 thanh ghi).
- Tối ưu stack: Thay vì stack 64 phần tử, trên các scene vừa phải (độ sâu cây BVH thấp), giảm xuống `stack[32]` hoặc `stack[24]`.

### 2. Russian Roulette (Xác suất sinh tồn)
Trên GPU, việc bắt một tia đập đủ 10 lần (max_depth) khi mà sau 3 lần nó đã tối om (attenuation gần bằng 0) là vô ích.
PBRT có đề cập đến Russian Roulette. Cách áp dụng vào CUDA loop:

```cuda
// Sau mỗi lần dội (Bắt đầu từ depth thứ 3 để đảm bảo ảnh sáng)
if (depth > 3) {
    float p = max(cur_attenuation.x, max(cur_attenuation.y, cur_attenuation.z)); // Khả năng sống sót
    if (curand_uniform(&local_rand_state) > p) {
        break; // Tia "chết", kết thúc sớm để tiết kiệm xung nhịp GPU
    }
    cur_attenuation = cur_attenuation * (1.0f / p); // Bù đắp ánh sáng cho những tia còn sống
}
```

### 3. Hướng tiếp cận tương lai (Wavefront Path Tracing)
*Đây là kiến thức chuyên sâu nâng cao.*
Megakernel gặp vấn đề Warp Divergence quá nặng (như đã nói ở chương 1). Cách giải quyết cấp độ công nghiệp (như Larrabee hay các engine AAA) là **Wavefront Path Tracing**.
Thay vì 1 Megakernel khổng lồ chạy từ đầu đến cuối cho 1 tia, bạn chia thành nhiều kernel nhỏ cực nhẹ:
- Kernel 1: Chỉ làm nhiệm vụ sinh tia.
- Kernel 2: Chỉ làm nhiệm vụ check Intersection (BVH) và đẩy ID vật liệu chạm phải vào 1 mảng.
- Kernel 3: Gom (Sort / Stream Compaction) các tia đập vào cùng 1 loại vật liệu (VD: gom tất cả tia đập vào kính lại).
- Kernel 4, 5, 6: Các kernel chuyên biệt tính shading cho Từng Loại Vật Liệu (1 kernel xử lý Kính, 1 kernel xử lý Nhựa...). Điều này **triệt tiêu hoàn toàn Warp Divergence**.
*Chú ý:* Việc này quá phức tạp cho người mới bắt đầu. Hãy thành thạo Megakernel trước khi bước vào Wavefront.

---
**KẾT LUẬN**
Bạn đang nắm trong tay bí quyết để chuyển hóa lượng lý thuyết đồ sộ của PBRT thành một cỗ máy phần mềm vắt kiệt sức mạnh GPU. Mọi công việc bây giờ là kiên trì thiết lập hệ thống build CMake, và viết lại từng struct toán học một cách chỉnh chu. Chúc bạn thành công với ScratchPathTracerCUDA!
