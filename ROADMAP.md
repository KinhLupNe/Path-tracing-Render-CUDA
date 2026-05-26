# Lộ trình triển khai Path Tracer trên CUDA (GTX 1650)

> Tài liệu này hướng dẫn chi tiết toàn bộ kiến thức, tài nguyên, và thứ tự cần học để xây dựng một path tracer chạy trên GPU GTX 1650 bằng CUDA, dựa trên series "Ray Tracing in One Weekend".

---

## Mục lục

1. [Tổng quan & Mục tiêu](#0-tổng-quan--mục-tiêu)
2. [Phase 1: Kiến thức nền tảng (bắt buộc trước khi bắt đầu)](#phase-1-kiến-thức-nền-tảng)
3. [Phase 2: Cài đặt môi trường](#phase-2-cài-đặt-môi-trường)
4. [Phase 3: CUDA cơ bản](#phase-3-cuda-cơ-bản)
5. [Phase 4: Ray Tracing trên CPU (RTIOW Book 1)](#phase-4-ray-tracing-trên-cpu)
6. [Phase 5: Port sang CUDA](#phase-5-port-sang-cuda)
7. [Phase 6: Monte Carlo đúng nghĩa (RTIOW Book 3)](#phase-6-monte-carlo-đúng-nghĩa)
8. [Phase 7: Mở rộng tính năng (RTIOW Book 2)](#phase-7-mở-rộng-tính-năng)
9. [Phase 8: Profiling & Tối ưu](#phase-8-profiling--tối-ưu)
10. [Phase 9: Nâng cao (tùy chọn)](#phase-9-nâng-cao-tùy-chọn)
11. [Cấu trúc project đề xuất](#cấu-trúc-project-đề-xuất)
12. [Các lỗi thường gặp](#các-lỗi-thường-gặp)
13. [Checklist tổng](#checklist-tổng)

---

## 0. Tổng quan & Mục tiêu

### Mục tiêu cuối cùng

Một chương trình CUDA render được scene cuối của RTIOW Book 1 (khoảng 488 quả cầu với 3 loại material: lambertian, metal, dielectric) ở độ phân giải 1200x800, 500 samples/pixel, trong khoảng **5-30 giây** trên GTX 1650.

Mở rộng (nếu có thời gian): BVH, textures, lights, volumes.

### Thông số GTX 1650 cần nhớ

| Thông số | Giá trị |
|---|---|
| Architecture | Turing (TU117) |
| Compute Capability | 7.5 |
| CUDA Cores | 896 |
| SMs (Streaming Multiprocessors) | 14 |
| VRAM | 4 GB GDDR5/GDDR6 |
| RT Cores | KHÔNG có |
| Tensor Cores | KHÔNG có |
| Compile flag | `-arch=sm_75` |
| Driver mode | WDDM (có watchdog 2 giây) |

### Triết lý học

- **Học theo nhu cầu**, không học trước. Mỗi vấn đề thực tế gặp phải sẽ dạy bạn một thứ.
- **Có cái chạy được trước, tối ưu sau**. Naive code chạy được > optimized code không chạy được.
- **Đo trước khi tối ưu**. Không guess performance, dùng profiler.

---

## Phase 1: Kiến thức nền tảng

Trước khi đụng CUDA, đảm bảo bạn nắm vững những thứ sau.

### 1.1 C++ cần biết

Mức độ yêu cầu: **trung cấp**, KHÔNG cần advanced.

- Pointers, references
- Classes, constructors, destructors
- Templates cơ bản (function templates và class templates đơn giản)
- `std::vector`, `std::unique_ptr` (sẽ KHÔNG dùng trên GPU nhưng dùng trên CPU)
- Operator overloading (cho vec3)

Không cần:
- Move semantics phức tạp
- SFINAE, concepts
- Multiple inheritance, virtual diamonds

**Tài liệu**:
- Nếu cần ôn lại: [learncpp.com](https://www.learncpp.com/) — chỉ phần 1-15.

### 1.2 Toán cần biết

- **Vector 3D**: dot product, cross product, length, normalize
- **Phương trình mặt cầu** và **giao điểm tia-cầu** (phương trình bậc 2)
- **Phản xạ (reflect)**: công thức `r = d - 2(d·n)n`
- **Khúc xạ (refract)**: định luật Snell
- **Schlick approximation** cho Fresnel
- **Hệ tọa độ camera** (eye, target, up, forward, right)

Không cần biết trước:
- Monte Carlo integration (sẽ học ở Phase 6)
- PDF, importance sampling (Phase 6)
- Linear algebra cao cấp (eigenvalue, SVD,...)

**Tài liệu**:
- Series video "Essence of Linear Algebra" của 3Blue1Brown (YouTube) — chỉ 6 video đầu.
- Các công thức toán trong RTIOW Book 1 đã giải thích đủ cho việc cài đặt.

### 1.3 Khái niệm Ray Tracing cần biết trước

- Ray = origin + t * direction
- Ray tracing vs Path tracing (khác nhau ở chỗ path tracing dùng Monte Carlo cho global illumination)
- Pinhole camera model
- Pixel = trung bình của nhiều samples (anti-aliasing)

Đọc qua: [Scratchapixel "Introduction to Ray Tracing"](https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing/) — 1-2 giờ.

---

## Phase 2: Cài đặt môi trường

Thời gian: 1-2 giờ.

### 2.1 Phần mềm cần cài

1. **Visual Studio 2022 Community** (miễn phí)
   - Cài workload "Desktop development with C++"
   - PHẢI cài trước CUDA Toolkit

2. **CUDA Toolkit 12.x** ([tải từ NVIDIA](https://developer.nvidia.com/cuda-downloads))
   - Phiên bản 12.0+ hoạt động tốt với GTX 1650
   - Khi cài, chọn "Custom" và đảm bảo có:
     - CUDA Runtime
     - CUDA Samples
     - Nsight Compute
     - Nsight Systems
     - Visual Studio Integration

3. **CMake 3.20+** ([cmake.org](https://cmake.org/download/))
   - Thêm vào PATH khi cài

4. **(Tùy chọn) Git** để version control project của bạn

### 2.2 Kiểm tra cài đặt

Mở PowerShell:

```powershell
nvcc --version          # Kiểm tra CUDA compiler
nvidia-smi              # Kiểm tra driver và GPU
cmake --version
```

`nvidia-smi` phải hiển thị "GeForce GTX 1650" và CUDA Version >= 12.

### 2.3 Build và chạy CUDA sample

```powershell
# Copy CUDA samples về thư mục có quyền ghi (Documents chẳng hạn)
# Mở solution deviceQuery trong Visual Studio, build & run
# Hoặc dùng CMake:
git clone https://github.com/NVIDIA/cuda-samples.git
cd cuda-samples/Samples/1_Utilities/deviceQuery
# Mở .vcxproj bằng VS, build, chạy
```

Output phải hiển thị thông tin GTX 1650 với CC 7.5. Nếu được, bạn đã sẵn sàng.

### 2.4 Nsight tools (sẽ dùng ở Phase 8)

- **Nsight Compute**: profile kernel chi tiết (occupancy, divergence, memory)
- **Nsight Systems**: profile toàn bộ application timeline

Đã cài cùng CUDA Toolkit. Chưa cần dùng vội.

---

## Phase 3: CUDA cơ bản

Thời gian: 2-4 ngày (đọc song song với code).

### 3.1 Các khái niệm BẮT BUỘC phải hiểu

#### Hierarchy thực thi
- **Thread**: đơn vị nhỏ nhất, chạy 1 lần kernel function
- **Warp**: 32 threads thực thi cùng lệnh (SIMT)
- **Block**: nhóm threads chia sẻ shared memory, có thể sync (`__syncthreads()`)
- **Grid**: tập hợp các blocks của 1 kernel launch

Khi launch kernel: `kernel<<<grid_size, block_size>>>(...)`. Mỗi block có size là bội số của 32 (thường 64, 128, 256).

#### Memory model cơ bản
- **Global memory**: VRAM, lớn (4GB trên GTX 1650), chậm (~400 cycles)
- **Shared memory**: per-block, nhanh (~30 cycles), nhỏ (48KB/block trên CC 7.5)
- **Registers**: per-thread, nhanh nhất, hạn chế (255 register/thread max)
- **Constant memory**: read-only, cached, 64KB tổng

Ở RTIOW level, bạn chủ yếu dùng **global memory** + **registers**. Shared memory chỉ động đến khi làm BVH (Phase 7).

#### Lifecycle dữ liệu
```cpp
// 1. Cấp phát trên host
float* h_data = new float[N];

// 2. Cấp phát trên device
float* d_data;
cudaMalloc(&d_data, N * sizeof(float));

// 3. Copy host → device
cudaMemcpy(d_data, h_data, N * sizeof(float), cudaMemcpyHostToDevice);

// 4. Chạy kernel
kernel<<<blocks, threads>>>(d_data);
cudaDeviceSynchronize();  // chờ kernel xong

// 5. Copy device → host
cudaMemcpy(h_data, d_data, N * sizeof(float), cudaMemcpyDeviceToHost);

// 6. Giải phóng
cudaFree(d_data);
delete[] h_data;
```

#### Function qualifiers
- `__global__`: kernel, gọi từ host, chạy trên device
- `__device__`: gọi từ device, chạy trên device (helper functions)
- `__host__`: chạy trên host (mặc định)
- `__host__ __device__`: compile cho cả 2 (dùng cho vec3, ray, hit_record,...)

#### Error checking
LUÔN check lỗi CUDA. Tạo macro:

```cpp
#define CUDA_CHECK(call) do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA error %s:%d: %s\n", __FILE__, __LINE__, \
                cudaGetErrorString(err)); \
        exit(1); \
    } \
} while(0)
```

Sau mỗi kernel launch:
```cpp
kernel<<<...>>>(...);
CUDA_CHECK(cudaGetLastError());
CUDA_CHECK(cudaDeviceSynchronize());
```

### 3.2 Tài liệu đọc

**Bắt buộc** (đọc theo thứ tự):

1. **NVIDIA Blog: "An Even Easier Introduction to CUDA"** ([link](https://developer.nvidia.com/blog/even-easier-introduction-cuda/))
   - 30 phút. Code "add 2 arrays" trên GPU. Đọc xong là chạy được kernel đầu tiên.

2. **NVIDIA Blog: "How to Implement Performance Metrics in CUDA C/C++"** ([link](https://developer.nvidia.com/blog/how-implement-performance-metrics-cuda-cc/))
   - 30 phút. Học cách đo thời gian kernel, hiểu bandwidth.

3. **CUDA C++ Programming Guide — chỉ chương 2-3** ([link](https://docs.nvidia.com/cuda/cuda-c-programming-guide/))
   - Skim qua, không cần đọc hết. Tham khảo khi cần.

**Không cần đọc bây giờ**: chương 4+ của Programming Guide, sách Kirk & Hwu, sách Wen-mei Hwu.

### 3.3 Bài tập trước khi sang Phase 4

Viết được những kernel sau (mỗi cái 1 file riêng):

1. **Hello CUDA**: in "Hello from thread (x,y)" từ kernel.
2. **Vector add**: cộng 2 mảng 1 triệu phần tử trên GPU, so sánh với CPU.
3. **Image gradient**: viết kernel sinh ảnh gradient RGB (mỗi pixel = 1 thread), ghi ra file PPM.

Bài 3 chính là Chapter 2 của RTIOW nhưng đã chạy trên GPU. Hoàn thành được là sẵn sàng cho path tracing.

---

## Phase 4: Ray Tracing trên CPU

Thời gian: 3-5 ngày.

### Tại sao làm trên CPU trước?

- Debug dễ hơn 10 lần (printf, debugger, breakpoint hoạt động bình thường)
- Hiểu thuật toán trước, không bị distract bởi CUDA
- Có baseline để so sánh: "CPU mất X giây, GPU mất Y giây"
- Code C++ có thể reuse khoảng 60-70% sang CUDA

### 4.1 Đọc và code RTIOW Book 1

Sách: **"Ray Tracing in One Weekend"** by Peter Shirley
- Link: https://raytracing.github.io/books/RayTracingInOneWeekend.html
- Miễn phí, online
- Khoảng 60 trang

Code theo từng chapter, KHÔNG nhảy bước:

| Chapter | Nội dung | Output |
|---|---|---|
| 1-2 | Output ảnh PPM | Ảnh gradient |
| 3 | Class vec3 | (utility) |
| 4 | Ray, camera đơn giản | Background gradient |
| 5 | Ray-sphere intersection | 1 quả cầu màu đỏ |
| 6 | Surface normals | Quả cầu màu theo normal |
| 7 | Hittable abstraction | Nhiều quả cầu |
| 8 | Antialiasing | Sharp edges biến mất |
| 9 | Diffuse materials | Quả cầu xám có bóng mềm |
| 10 | Metal | Quả cầu kim loại |
| 11 | Dielectrics | Quả cầu thủy tinh |
| 12 | Camera với defocus blur | Depth of field |
| 13 | Final scene | 488 quả cầu, ảnh kinh điển |

### 4.2 Lưu ý quan trọng cho việc port sang CUDA sau này

Trong khi code Book 1 trên CPU, **CHỦ Ý thiết kế** để dễ port:

1. **Tách `Material` interface ra sớm**, thiết kế trả về có chỗ cho PDF (Book 3) ngay cả khi Book 1 chưa dùng. Tránh refactor lớn sau này.

2. **Đặt vec3 ops `inline` và `constexpr`**: vec3.h sẽ thành `__host__ __device__` sau này.

3. **TRÁNH dùng `std::shared_ptr` trong scene**: nếu có thể, dùng `std::vector<unique_ptr<Hittable>>` hoặc raw pointers. shared_ptr không port được lên GPU.

4. **Tách `ray_color` thành iterative ngay từ đầu**:
```cpp
// Thay vì đệ quy như sách
color ray_color_iter(ray r, const hittable& world, int max_depth) {
    color throughput(1,1,1);
    color result(0,0,0);
    for (int i = 0; i < max_depth; i++) {
        hit_record rec;
        if (!world.hit(r, 0.001, infinity, rec)) {
            result += throughput * background(r);
            break;
        }
        ray scattered;
        color attenuation;
        if (!rec.mat->scatter(r, rec, attenuation, scattered)) break;
        throughput *= attenuation;
        r = scattered;
    }
    return result;
}
```
Cách này SẼ chạy trên GPU. Đệ quy thì KHÔNG.

5. **Lưu các tham số render** (image_width, samples_per_pixel, max_depth) ở một struct/class, dễ pass vào kernel sau này.

### 4.3 Mục tiêu cuối Phase 4

- Render được scene cuối Book 1 trên CPU.
- Thời gian render: ghi lại để so sánh sau (sẽ rất chậm, có thể vài phút - vài giờ tùy cài đặt).
- Code có cấu trúc gọn gàng, sẵn sàng port sang CUDA.

---

## Phase 5: Port sang CUDA

Thời gian: 5-10 ngày. Đây là phần khó nhất của project.

### 5.1 Đọc tài liệu chính

**Bắt buộc đọc**: NVIDIA Blog "Accelerated Ray Tracing in One Weekend in CUDA" ([link](https://developer.nvidia.com/blog/accelerated-ray-tracing-cuda/))

Đây là tutorial CHÍNH THỨC từ NVIDIA, port nguyên RTIOW Book 1 sang CUDA. Đọc cẩn thận, hiểu từng dòng. Source code: https://github.com/rogerallen/raytracinginoneweekendincuda

### 5.2 Các vấn đề kỹ thuật và cách giải

#### Vấn đề 1: Virtual functions và inheritance

`shared_ptr` không dùng được trên device. `new`/`delete` trên device chạy được nhưng chậm.

**Cách 1 (theo NVIDIA blog)**: dùng `new` trên device trong một kernel khởi tạo riêng:
```cpp
__global__ void create_world(hittable** d_list, hittable** d_world, ...) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        d_list[0] = new sphere(vec3(0,0,-1), 0.5, new lambertian(...));
        // ...
        *d_world = new hittable_list(d_list, n);
    }
}
```

**Cách 2 (tốt hơn)**: tagged union, không dùng virtual:
```cpp
struct Material {
    enum Type { LAMBERTIAN, METAL, DIELECTRIC } type;
    vec3 albedo;
    float fuzz;        // metal
    float ref_idx;     // dielectric
};

__device__ bool scatter(const Material& m, const ray& r, const hit_record& rec,
                        vec3& attenuation, ray& scattered, curandState* rng) {
    switch (m.type) {
        case Material::LAMBERTIAN: return scatter_lambertian(m, rec, attenuation, scattered, rng);
        case Material::METAL:      return scatter_metal(m, r, rec, attenuation, scattered, rng);
        case Material::DIELECTRIC: return scatter_dielectric(m, r, rec, attenuation, scattered, rng);
    }
    return false;
}
```

Khuyến nghị **Cách 2** vì:
- Không cần allocate trên device
- Build scene trên host, chỉ memcpy lên device
- Tránh virtual function call (giảm warp divergence)

#### Vấn đề 2: Random numbers

Dùng `curand`:

```cpp
#include <curand_kernel.h>

__global__ void init_rng(curandState* states, int width, int height, unsigned long seed) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    int idx = y * width + x;
    curand_init(seed + idx, 0, 0, &states[idx]);
}

__device__ float random_float(curandState* rng) {
    return curand_uniform(rng);  // [0, 1)
}
```

Mỗi pixel có 1 `curandState` riêng. Cấp phát: `cudaMalloc(&d_rng, width * height * sizeof(curandState))`.

#### Vấn đề 3: WDDM watchdog 2 giây

Trên Windows, OS sẽ kill kernel chạy > 2 giây. Giải pháp:

1. **Chia samples per pixel thành nhiều launches**:
```cpp
for (int s = 0; s < samples_per_pixel; s += batch_size) {
    render_kernel<<<grid, block>>>(d_accum, d_rng, batch_size, ...);
    CUDA_CHECK(cudaDeviceSynchronize());
}
```
2. **Chia ảnh thành tiles** và render từng tile.
3. Mỗi launch < 1 giây để an toàn.

#### Vấn đề 4: vec3 cho cả host và device

```cpp
class vec3 {
public:
    __host__ __device__ vec3() : e{0,0,0} {}
    __host__ __device__ vec3(float x, float y, float z) : e{x,y,z} {}
    __host__ __device__ float x() const { return e[0]; }
    // ... mọi method đều __host__ __device__
private:
    float e[3];
};
```

#### Vấn đề 5: ray_color phải iterative

Đã làm sẵn ở Phase 4. Nhớ thay `std::random` bằng `curand_uniform`.

### 5.3 Cấu trúc kernel chính

```cpp
__global__ void render_kernel(
    vec3* fb,                    // framebuffer
    int width, int height,
    int samples_per_pixel,
    int max_depth,
    Camera cam,
    Sphere* spheres, int n_spheres,
    Material* materials,
    curandState* rng_states
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    
    int pixel_idx = y * width + x;
    curandState rng = rng_states[pixel_idx];
    
    vec3 color(0, 0, 0);
    for (int s = 0; s < samples_per_pixel; s++) {
        float u = (x + curand_uniform(&rng)) / width;
        float v = (y + curand_uniform(&rng)) / height;
        ray r = cam.get_ray(u, v, &rng);
        color += ray_color_iter(r, spheres, n_spheres, materials, &rng, max_depth);
    }
    color /= samples_per_pixel;
    
    rng_states[pixel_idx] = rng;  // lưu lại state
    fb[pixel_idx] = color;
}
```

Launch config:
```cpp
dim3 block(16, 16);  // 256 threads/block
dim3 grid((width + 15) / 16, (height + 15) / 16);
render_kernel<<<grid, block>>>(...);
```

### 5.4 Mục tiêu cuối Phase 5

- Scene cuối RTIOW Book 1 render được trên GPU.
- Thời gian render: 5-30 giây (so với vài phút - vài giờ trên CPU).
- Output PPM hoặc PNG, ảnh giống hệt CPU version.

---

## Phase 6: Monte Carlo đúng nghĩa

Thời gian: 5-7 ngày.

### Tại sao Phase này?

Book 1 dùng "fake BSDF" — kết quả nhìn đúng nhưng toán học sai. Phase này sửa lại, đồng thời cải thiện convergence (ít noise hơn với cùng số samples).

### 6.1 Đọc RTIOW Book 3

Sách: **"Ray Tracing: The Rest of Your Life"**
- Link: https://raytracing.github.io/books/RayTracingTheRestOfYourLife.html
- Khoảng 50 trang, NẶNG về toán

Sẽ học:
- Monte Carlo integration
- Probability Density Function (PDF)
- Importance sampling
- Cosine-weighted sampling
- Light sampling (sampling toward lights)
- Mixture PDFs (Multiple Importance Sampling đơn giản)

### 6.2 Các khái niệm phải hiểu

#### Render equation
```
L_o(p, ω_o) = L_e(p, ω_o) + ∫ f(p, ω_i, ω_o) · L_i(p, ω_i) · (n · ω_i) dω_i
```

Path tracing = ước lượng tích phân này bằng Monte Carlo:
```
L ≈ L_e + (1/N) Σ [f(...) · L_i · cosθ / pdf(ω_i)]
```

#### BRDF của Lambertian (đúng)
- `f = albedo / π`
- `scattering_pdf = cosθ / π` (cosine-weighted)
- Khi sample đúng (cosine-weighted), nhiều thứ rút gọn:
  - `f · cosθ / pdf = (albedo/π) · cosθ / (cosθ/π) = albedo`
  - → đúng như Book 1 đã viết "tình cờ"!
- Nhưng phải sample ĐÚNG (cosine-weighted), không phải uniform on sphere.

#### Cosine-weighted sampling
```cpp
__device__ vec3 random_cosine_direction(curandState* rng) {
    float r1 = curand_uniform(rng);
    float r2 = curand_uniform(rng);
    float phi = 2 * M_PI * r1;
    float x = cos(phi) * sqrt(r2);
    float y = sin(phi) * sqrt(r2);
    float z = sqrt(1 - r2);
    return vec3(x, y, z);  // ở local frame, z là normal
}
```

Sau đó transform về world frame bằng orthonormal basis (ONB).

### 6.3 Refactor material interface

```cpp
struct ScatterRecord {
    vec3 attenuation;
    bool skip_pdf;          // true cho specular (metal, dielectric)
    ray skip_pdf_ray;       // dùng khi skip_pdf
    // Cho non-specular: sample direction từ cosine_pdf
};

__device__ bool scatter_v2(const Material& m, const ray& r_in, const hit_record& rec,
                            ScatterRecord& srec, curandState* rng);

__device__ float scattering_pdf(const Material& m, const ray& r_in,
                                 const hit_record& rec, const ray& scattered);
```

Vòng lặp ray_color trở thành:
```cpp
for (int depth = 0; depth < max_depth; depth++) {
    if (!hit) { radiance += throughput * background; break; }
    radiance += throughput * emitted(material, rec);
    
    ScatterRecord srec;
    if (!scatter_v2(material, r, rec, srec, &rng)) break;
    
    if (srec.skip_pdf) {
        throughput *= srec.attenuation;
        r = srec.skip_pdf_ray;
    } else {
        vec3 dir = random_cosine_direction(&rng);  // hoặc mixture với light sampling
        ray scattered(rec.p, dir);
        float pdf_val = cosine_pdf(rec.normal, dir);
        float scat_pdf = scattering_pdf(material, r, rec, scattered);
        throughput *= srec.attenuation * scat_pdf / pdf_val;
        r = scattered;
    }
    // Russian Roulette
    if (depth > 3) {
        float p = max3(throughput);
        if (curand_uniform(&rng) > p) break;
        throughput /= p;
    }
}
```

### 6.4 Russian Roulette

Kỹ thuật unbiased để terminate ray sớm khi throughput thấp:
```cpp
if (depth > 3) {
    float p = max(throughput.x, max(throughput.y, throughput.z));
    p = min(p, 0.95f);
    if (curand_uniform(rng) > p) break;
    throughput /= p;  // bù lại
}
```

Giảm số bounces trung bình → tăng tốc, không bias kết quả.

### 6.5 Mục tiêu cuối Phase 6

- Same scene cuối Book 1, nhưng dùng đúng Monte Carlo.
- Convergence tốt hơn (cùng 500 samples nhưng ít noise hơn).
- Code material đã hỗ trợ PDF, sẵn sàng cho lights ở Phase 7.

---

## Phase 7: Mở rộng tính năng

Thời gian: 7-14 ngày (tùy phạm vi).

### 7.1 Đọc RTIOW Book 2

Sách: **"Ray Tracing: The Next Week"**
- Link: https://raytracing.github.io/books/RayTracingTheNextWeek.html

Các tính năng (chọn cái bạn muốn, không cần làm hết):

| Feature | Độ khó CUDA | Đáng làm? |
|---|---|---|
| Motion blur | Dễ | Nice-to-have |
| BVH | KHÓ (khó nhất Book 2) | Bắt buộc nếu > 100 objects |
| Solid textures (checker, perlin) | Dễ | Có |
| Image textures | Trung bình (texture memory) | Có |
| Lights | Trung bình (kết hợp Phase 6) | Có, đẹp ảnh |
| Quads | Dễ | Có cho Cornell Box |
| Instances (translate, rotate) | Dễ | Có |
| Volumes (smoke, fog) | Trung bình | Đẹp |

### 7.2 BVH trên GPU

BVH là phần khó nhất:
- Build trên CPU (đệ quy hoặc Morton-code based) rồi flatten thành array.
- Truy cập trên GPU bằng vòng lặp (KHÔNG đệ quy), dùng stack thủ công.

```cpp
__device__ bool bvh_hit(const BVHNode* nodes, const Sphere* spheres,
                        const ray& r, float t_min, float t_max,
                        hit_record& rec) {
    int stack[32];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;  // root
    bool hit_anything = false;
    float closest = t_max;
    while (stack_ptr > 0) {
        int node_idx = stack[--stack_ptr];
        const BVHNode& node = nodes[node_idx];
        if (!aabb_hit(node.box, r, t_min, closest)) continue;
        if (node.is_leaf) {
            if (sphere_hit(spheres[node.object_idx], r, t_min, closest, rec)) {
                hit_anything = true;
                closest = rec.t;
            }
        } else {
            stack[stack_ptr++] = node.left;
            stack[stack_ptr++] = node.right;
        }
    }
    return hit_anything;
}
```

Stack 32 là đủ cho cây cân bằng với 4 tỷ leaves.

### 7.3 Image textures với CUDA texture memory

Dùng `cudaTextureObject_t` cho hardware filtering & wrapping:
```cpp
cudaResourceDesc res_desc = {};
res_desc.resType = cudaResourceTypeArray;
res_desc.res.array.array = cu_array;

cudaTextureDesc tex_desc = {};
tex_desc.addressMode[0] = cudaAddressModeWrap;
tex_desc.filterMode = cudaFilterModeLinear;
tex_desc.normalizedCoords = 1;

cudaTextureObject_t tex_obj;
cudaCreateTextureObject(&tex_obj, &res_desc, &tex_desc, nullptr);
```

Trong kernel:
```cpp
float4 c = tex2D<float4>(tex_obj, u, v);
```

### 7.4 Mục tiêu cuối Phase 7

Tùy thời gian. Tối thiểu nên có: **BVH + lights + 1 loại texture**. Đủ để render Cornell Box hoặc scene phức tạp.

---

## Phase 8: Profiling & Tối ưu

Thời gian: 3-5 ngày.

### 8.1 Đo bằng Nsight Compute

```powershell
ncu --set full -o profile_output path-tracer.exe
```

Mở file `.ncu-rep` trong Nsight Compute GUI. Quan tâm:

| Metric | Ý nghĩa | Mục tiêu |
|---|---|---|
| **SM Active** | % SM bận | > 80% |
| **Achieved Occupancy** | % warp slot đang dùng | > 50% |
| **Warp Execution Efficiency** | % thread active trong warp | > 50% (path tracing khó cao hơn) |
| **Memory Throughput** | Bandwidth thực tế | So với 128 GB/s peak |
| **Registers per Thread** | Register pressure | < 64 để occupancy cao |

### 8.2 Các tối ưu thường gặp

#### Giảm register pressure
- Tách kernel lớn thành nhiều kernel nhỏ
- Compile với `-maxrregcount=64`
- Đưa các biến không cần persistent ra ngoài (recompute)

#### Tăng occupancy
- Giảm block size từ 256 xuống 128 hoặc 64
- Giảm shared memory dùng

#### Giảm divergence
- Sort rays theo material (mini-wavefront)
- Compact alive rays mỗi bounce

#### Memory coalescing
- Đảm bảo threads trong warp truy cập memory contiguous
- AoS → SoA cho hot data

### 8.3 Khi nào dừng tối ưu

Khi đạt 1 trong các điều kiện:
- Time per frame đã đủ cho mục đích của bạn
- Tối ưu thêm tốn nhiều effort hơn lợi ích
- Profile cho thấy bottleneck là memory bandwidth (không thể vượt qua được)

**KHÔNG** tối ưu vì "có thể tối ưu được". Tối ưu vì "cần tối ưu".

---

## Phase 9: Nâng cao (tùy chọn)

Chỉ làm khi đã xong tất cả phase trên và còn thời gian/đam mê.

### 9.1 Wavefront Path Tracing

Đọc theo thứ tự:
1. **Laine, Karras, Aila 2013**: "Megakernels Considered Harmful: Wavefront Path Tracing on GPUs" (paper)
2. **PBRT v4 Chapter 15**: "Wavefront Rendering on GPUs" (https://pbr-book.org/4ed/Wavefront_Rendering_on_GPUs)

Sẽ phải refactor lớn: tách megakernel thành nhiều kernel nhỏ (generate, intersect, shade), quản lý ray queues.

### 9.2 Đề tài khác có thể khám phá

- **Bidirectional Path Tracing**: trace từ cả camera và light
- **Metropolis Light Transport**: importance sampling theo MCMC
- **Photon Mapping**
- **Denoising** (SVGF, A-SVGF, hoặc dùng OIDN/OptiX denoiser)
- **Real-time rendering**: NEE (Next Event Estimation), ReSTIR

### 9.3 Sách & tài liệu để đi sâu hơn

- **Physically Based Rendering: From Theory to Implementation (4th ed.)** — Pharr, Jakob, Humphreys. Free: https://pbr-book.org/
- **Ray Tracing Gems I & II** (NVIDIA, free PDF)
- **Real-Time Rendering (4th ed.)** — Akenine-Möller, Haines, Hoffman
- **Advanced Global Illumination** — Dutré, Bekaert, Bala

---

## Cấu trúc project đề xuất

```
ScratchPathTracerCUDA/
├── CMakeLists.txt
├── ROADMAP.md                  (file này)
├── README.md
├── src/
│   ├── main.cpp                (entry point, parse args, save image)
│   ├── core/
│   │   ├── vec3.h              (__host__ __device__)
│   │   ├── ray.h
│   │   ├── color.h
│   │   └── utils.h             (random, math helpers)
│   ├── camera/
│   │   └── camera.h
│   ├── geometry/
│   │   ├── hittable.h          (tagged union)
│   │   ├── sphere.h
│   │   ├── aabb.h
│   │   └── bvh.h               (Phase 7)
│   ├── material/
│   │   ├── material.h          (tagged union)
│   │   ├── pdf.h               (Phase 6)
│   │   └── texture.h           (Phase 7)
│   ├── render/
│   │   ├── render_kernel.cu    (kernel chính)
│   │   ├── path_tracer.cu      (host-side launcher)
│   │   └── ray_color.h         (device function)
│   ├── scene/
│   │   ├── scene.h             (scene builder)
│   │   └── scenes.cpp          (preset scenes: book1_final, cornell_box,...)
│   └── io/
│       ├── image_writer.h      (PPM, PNG)
│       └── timer.h
├── third_party/
│   └── stb_image_write.h       (single-header PNG writer)
└── output/                     (folder cho ảnh render)
```

### CMakeLists.txt mẫu (skeleton)

```cmake
cmake_minimum_required(VERSION 3.20)
project(ScratchPathTracerCUDA LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES 75)  # GTX 1650

add_executable(path_tracer
    src/main.cpp
    src/render/path_tracer.cu
    src/render/render_kernel.cu
    src/scene/scenes.cpp
)

target_include_directories(path_tracer PRIVATE src third_party)
set_target_properties(path_tracer PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

target_compile_options(path_tracer PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>
    $<$<COMPILE_LANGUAGE:CUDA>:-lineinfo>
)
```

---

## Các lỗi thường gặp

### "CUDA error: an illegal memory access was encountered"
- Truy cập ngoài mảng trên device
- Quên `cudaMalloc`
- Truyền pointer host vào kernel (phải truyền pointer device)
- Index tính sai (kiểm tra `if (x >= width || y >= height) return;`)

**Debug**: compile với `-G` (debug info) và chạy với `compute-sanitizer your_program.exe`. Sẽ chỉ chính xác dòng lỗi.

### Kernel chạy nhưng output đen
- Quên `cudaDeviceSynchronize()` trước khi copy về
- RNG state chưa init
- Sai logic, nhưng debug bằng `printf` từ kernel:
```cpp
if (x == 0 && y == 0) printf("color = %f %f %f\n", c.x, c.y, c.z);
```

### Kernel chạy quá lâu, OS kill
- WDDM watchdog. Chia thành nhiều launches < 1 giây.
- Mỗi launch render 1 batch samples hoặc 1 tile.

### "too many resources requested for launch"
- Register pressure quá cao. Block size 256 không đủ register.
- Giảm block size xuống 128 hoặc 64.
- Hoặc thêm `-maxrregcount=64` vào nvcc.

### Ảnh có pattern lạ (sọc, lưới)
- RNG sai (cùng seed cho mọi pixel)
- Init `curand_init(seed + pixel_idx, 0, 0, &state)`

### Race condition trên framebuffer
- Nhiều thread ghi cùng 1 pixel (sai thread mapping)
- Mỗi pixel = 1 thread duy nhất

### Build chậm (5-10 phút mỗi lần đổi 1 dòng)
- Tránh `--use_fast_math` khi đang dev (chỉ enable lúc release)
- Đặt scene definitions ở `.cpp` không phải `.cu`
- Dùng `CUDA_SEPARABLE_COMPILATION ON` để chỉ recompile file thay đổi

---

## Checklist tổng

### Trước khi bắt đầu
- [ ] Hiểu C++ trung cấp (class, template, pointer)
- [ ] Hiểu vector 3D, dot/cross product
- [ ] Đã đọc scratchapixel intro ray tracing
- [ ] Visual Studio 2022 cài xong
- [ ] CUDA Toolkit cài xong, `nvcc --version` chạy được
- [ ] `nvidia-smi` hiển thị GTX 1650 + CUDA version
- [ ] Build và chạy được `deviceQuery` sample

### Phase 3 — CUDA cơ bản
- [ ] Đọc xong "An Even Easier Introduction to CUDA"
- [ ] Viết được Hello CUDA kernel
- [ ] Viết được vector add
- [ ] Viết được image gradient output PPM

### Phase 4 — RTIOW Book 1 CPU
- [ ] Chapter 1-7: render được nhiều quả cầu
- [ ] Chapter 8-11: 3 materials hoạt động đúng
- [ ] Chapter 12: defocus blur
- [ ] Chapter 13: final scene
- [ ] Code đã iterative (không đệ quy)
- [ ] vec3 dùng `inline`, sẵn sàng thêm `__host__ __device__`

### Phase 5 — Port CUDA
- [ ] Đọc xong NVIDIA blog "Accelerated Ray Tracing in CUDA"
- [ ] Hello CUDA path tracer (1 quả cầu)
- [ ] Material dùng tagged union
- [ ] curand hoạt động đúng
- [ ] Scene cuối Book 1 render trên GPU
- [ ] Thời gian render ghi nhận: ___ giây

### Phase 6 — Monte Carlo đúng
- [ ] Đọc xong RTIOW Book 3
- [ ] Material trả về scatter_record có PDF
- [ ] Cosine-weighted sampling
- [ ] Russian Roulette
- [ ] Convergence cải thiện (so sánh noise với Phase 5)

### Phase 7 — Mở rộng
- [ ] BVH build trên CPU, traverse trên GPU
- [ ] Ít nhất 1 loại texture
- [ ] Lights + light sampling
- [ ] Render được Cornell Box hoặc scene phức tạp

### Phase 8 — Profile & tối ưu
- [ ] Chạy Nsight Compute, đọc được report
- [ ] Đo occupancy, biết tại sao
- [ ] Đo warp efficiency, biết divergence ở đâu
- [ ] Thử 1-2 tối ưu, đo kết quả

### Phase 9 — Optional
- [ ] Đọc Laine 2013 (nếu tò mò)
- [ ] Đọc PBRT v4 Ch.15 (nếu muốn đi sâu)
- [ ] Cài wavefront hoặc denoising

---

## Tổng kết thời gian ước tính

| Phase | Thời gian | Khó hay dễ |
|---|---|---|
| 1-2 (nền tảng, môi trường) | 1 tuần | Dễ |
| 3 (CUDA cơ bản) | 1 tuần | Trung bình |
| 4 (RTIOW Book 1 CPU) | 1-2 tuần | Trung bình |
| 5 (Port CUDA) | 2-3 tuần | **Khó nhất** |
| 6 (Monte Carlo) | 1-2 tuần | Khó (toán) |
| 7 (Mở rộng) | 2-4 tuần | Trung bình - khó |
| 8 (Profile) | 1 tuần | Trung bình |
| 9 (Optional) | tùy | Khó |

**Tổng cho mục tiêu cơ bản (Phase 1-5)**: ~6 tuần làm việc bán thời gian.
**Tổng cho project hoàn chỉnh (Phase 1-7)**: ~3 tháng.

---

## Lời khuyên cuối

1. **Đừng cầu toàn**. Code chạy được rồi sửa, hơn là design hoàn hảo trên giấy.
2. **Commit thường xuyên**. Mỗi chapter của RTIOW là 1 commit. Khi sai có thể rollback.
3. **Đừng nhảy phase**. Mỗi phase đều có lý do tồn tại trong thứ tự đó.
4. **Khi bí, đọc code người khác**: https://github.com/rogerallen/raytracinginoneweekendincuda là tham khảo tốt.
5. **Đo trước khi tối ưu**. Nsight Compute là bạn.
6. **Documenting your journey**: viết blog/note về những gì học được — ép bạn suy nghĩ rõ ràng và là portfolio sau này.

Chúc may mắn.
