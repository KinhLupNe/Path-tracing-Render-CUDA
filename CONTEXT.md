# TRẠNG THÁI DỰ ÁN: CUDA PATH TRACER (ScratchPathTracerCUDA)

*Tài liệu này dùng để "nạp" lại bộ nhớ cho AI khi bắt đầu một phiên chat mới, giúp AI hiểu ngay lập tức kiến trúc và tiến độ hiện tại mà không cần đọc lại toàn bộ mã nguồn.*

## 1. TIẾN ĐỘ HIỆN TẠI
- **Lộ trình:** Đã hoàn thành Giai đoạn khởi tạo (C++ & CUDA Setup) và Render thành công Hình cầu đầu tiên (Sphere) với bề mặt Normal shading.
- **Tiến độ tương ứng với sách RTIOW:** Đang ở Chương 4 (Tạo tia & Camera) & Chương 5, 6 (Hình cầu & Surface Normals).
- **Giao diện:** Đã tích hợp thành công OpenGL PBO (Pixel Buffer Object) + ImGui.
- **Tính năng UI:** Đã có thanh trượt điều khiển Vị trí `(X, Y, Z)` và `Bán kính` của quả cầu theo thời gian thực (60+ FPS).

## 2. KIẾN TRÚC MÃ NGUỒN
Dự án không dùng OOP phức tạp, ưu tiên Data-Oriented Design cho GPU:

1.  **`src/main.cpp` (The Host):**
    *   Khởi tạo GLFW Window, cấu hình OpenGL.
    *   Tạo PBO (Cầu nối Zero-Copy giữa CUDA và OpenGL) và `cudaGraphicsMapResources`.
    *   Chạy vòng lặp Render loop, cập nhật ImGui UI, truyền tham số (tâm, bán kính, time) xuống Kernel qua hàm wrapper.

2.  **`src/render_kernel.h` & `src/render_kernel.cu` (The Device / Megakernel):**
    *   `render_anim_kernel`: Kernel CUDA chính. Đã áp dụng chuẩn **Pixel Delta Mapping** cực kỳ hiện đại (tính `vdvw`, `vdvh` thay vì tính `u`, `v` thô).
    *   `ray_color`: Hàm tính màu hiện tại đang Hardcode 1 `Sphere s(...)` và tính toán giao điểm (Hit).
    *   Sử dụng cờ `-rdc=true` trong CMake để liên kết Separable Compilation. Đã cấu hình `.clangd` bỏ qua cờ này để Neovim không báo lỗi.

3.  **Hệ thống Toán học (`src/math/`):**
    *   `vec3.h`: Cấu trúc vector 3 chiều, dùng `__host__ __device__` hỗ trợ overload operator toán học.
    *   `ray.h`: Chứa Gốc (Origin) và Hướng (Direction), cộng hàm `at(t)`.
    *   `hittable/sphere.h`: Hàm giải phương trình giao điểm tia - cầu cực nhẹ.

## 3. CÔNG VIỆC TIẾP THEO (Tương lai)
1.  **Chương 8 (RTIOW):** Tích hợp thư viện `cuRAND` để sinh số ngẫu nhiên. Bắn nhiều tia lệch nhau trên mỗi pixel để thực hiện Khử răng cưa (Antialiasing - MSAA). Đã có bản Implementation Plan.
2.  **Thế giới đa vật thể:** Xây dựng cấu trúc `HittableList` (hoặc mảng hình học thô) cấp phát trên GPU để chứa hàng trăm quả cầu.
3.  **Vật liệu (Materials):** Cập nhật `ray_color` thành thuật toán nảy tia (Bounces) với vật liệu Nhám (Diffuse), Kim loại (Metal) và Kính (Dielectric).

## 4. QUY TẮC TƯƠNG TÁC (Đặc biệt lưu ý cho AI)
1.  **Môi trường:** Người dùng đang xài Windows, dùng trình soạn thảo **Neovim**, biên dịch bằng `run.bat` (chứa CMake build).
2.  **Sở thích Code:** Người dùng **VÔ CÙNG TỰ LẬP**. Cực ghét việc AI copy-paste code đáp sẵn. Người dùng thích được gợi ý logic, hướng dẫn thuật toán từng bước để **tự tay gõ code**. Chỉ cung cấp code hoàn chỉnh khi gặp lỗi quá hiểm hóc hoặc config hệ thống (như CMake).
3.  **Toán học:** Người dùng có tư duy hình học không gian cực tốt (tự viết được thuật toán Pixel Delta Vector), đừng đánh giá thấp khả năng C++ của họ!
