#include "render_kernel.h"

// Đây là "Hạt nhân" (Kernel) chạy trên hàng vạn nhân CUDA
// Từ khóa __global__ nghĩa là: CPU gọi lệnh này, nhưng GPU mới là thằng thực thi
__global__ void render_anim_kernel(int width, int height, uchar4* d_output, float time, float speed) {
    // ==============================================================
    // 1. TÍNH TỌA ĐỘ PIXEL TRÊN MÀN HÌNH
    // ==============================================================
    // GPU chia công việc thành các Block (Khối), mỗi Block có nhiều Thread (Luồng).
    // Công thức này giúp luồng hiện tại biết nó đang phụ trách tọa độ x, y nào trên bức ảnh
    int i = threadIdx.x + blockIdx.x * blockDim.x; // Cột (x)
    int j = threadIdx.y + blockIdx.y * blockDim.y; // Hàng (y)

    // Nếu luồng này tính lố ra ngoài viền màn hình thì bắt nó dừng lại để bảo vệ VRAM
    if (i >= width || j >= height) return;

    // Chuẩn hóa tọa độ x, y (đang từ 0->800) thành khoảng 0.0 -> 1.0 (như tỷ lệ phần trăm)
    float u = float(i) / float(width);
    float v = float(j) / float(height);

    // ==============================================================
    // 2. THUẬT TOÁN TÍNH MÀU SẮC
    // (Chỗ này sau này sẽ thay bằng Thuật toán Path Tracing lõi)
    // ==============================================================
    // Hiện tại chỉ là hàm sóng Cos/Sin tạo ra màu RGB gợn sóng thay đổi theo thời gian
    float r = 0.5f + 0.5f * cosf(time + u * speed);
    float g = 0.5f + 0.5f * sinf(time + v * speed);
    float b = 0.5f + 0.5f * cosf(time + (u+v) * (speed * 0.5f));

    // ==============================================================
    // 3. TÌM VỊ TRÍ TRONG BỘ NHỚ TUYẾN TÍNH
    // ==============================================================
    // Mảng d_output là mảng 1 chiều, nhưng màn hình là mảng 2 chiều.
    // Công thức (Hàng * ChiềuRộng + Cột) giúp chuyển tọa độ 2D thành Index 1D
    int pixel_index = j * width + i; 

    // ==============================================================
    // 4. GHI MÀU VÀO VRAM (Vào chính cái PBO của OpenGL)
    // ==============================================================
    // d_output là kiểu uchar4 (mỗi kênh màu từ 0-255) nên phải nhân màu thực (0.0->1.0) với 255.99
    d_output[pixel_index].x = (unsigned char)(255.99f * r); // Đỏ (Red)
    d_output[pixel_index].y = (unsigned char)(255.99f * g); // Xanh lá (Green)
    d_output[pixel_index].z = (unsigned char)(255.99f * b); // Xanh dương (Blue)
    d_output[pixel_index].w = 255;                          // Độ đặc (Alpha/Opacity)
}

// Đây là hàm vỏ bọc (Wrapper) chạy trên CPU để ra lệnh cho GPU
void launch_render_kernel(int width, int height, uchar4* d_output, float time, float speed) {
    // Định nghĩa 1 Block (Khối) sẽ chứa 8x8 = 64 luồng (threads)
    int tx = 8;
    int ty = 8;
    
    // Tính toán xem cần bao nhiêu Block để phủ kín toàn bộ màn hình (width x height)
    dim3 blocks(width / tx + 1, height / ty + 1);
    dim3 threads(tx, ty);

    // Kích hoạt "Công tắc phân thân" <<<blocks, threads>>>
    // Ra lệnh cho GPU xuất ra hàng trăm ngàn luồng chạy hàm render_anim_kernel cùng một lúc
    render_anim_kernel<<<blocks, threads>>>(width, height, d_output, time, speed);
    
    // Yêu cầu CPU phải đứng chờ GPU tính toán xong toàn bộ màn hình mới được chạy code tiếp.
    // Nếu thiếu dòng này, CPU có thể ra lệnh cho OpenGL vẽ trong khi CUDA còn chưa tô màu xong!
    cudaDeviceSynchronize(); 
}
