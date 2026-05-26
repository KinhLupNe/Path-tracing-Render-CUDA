# [CORE CONTEXT] CUDA PATH TRACER PROJECT

## 1. Mục tiêu dự án
Xây dựng một Trình Render Path Tracing thời gian thực (Real-time) hoàn toàn từ con số 0 bằng C++ và CUDA, dựa trên lý thuyết của sách PBRTv4 và cấu trúc của "Ray Tracing in One Weekend".

## 2. Phần cứng & Giới hạn
- **GPU:** NVIDIA GTX 1650 (Kiến trúc Turing, 4GB VRAM, KHÔNG có RT Cores).
- **Giới hạn sống còn:** Bộ nhớ Stack trên GPU rất nhỏ. TUYỆT ĐỐI KHÔNG DÙNG ĐỆ QUY (Recursion) sâu trong CUDA Kernel. Phải chuyển đổi các vòng lặp đệ quy nảy bật ánh sáng thành Iterative Loop (vòng lặp for/while).

## 3. Kiến trúc Phần mềm (ĐÃ THIẾT LẬP)
- **Boilerplate:** Đã cấu hình xong CMake, cửa sổ GLFW, và Dear ImGui (dùng backend OpenGL 2 Legacy).
- **Hiển thị:** Dùng công nghệ CUDA-OpenGL Interop (Zero-Copy PBO). CUDA tính toán màu sắc và ghi trực tiếp vào VRAM của OpenGL để hiển thị lên màn hình.
- **Môi trường Dev:** Neovim + clangd. Đã có file `.clangd` cấu hình để bỏ qua các cờ rác của `nvcc`.

## 4. Nguyên tắc Thiết kế (CỰC KỲ QUAN TRỌNG)
- **Data-Oriented Design (DOD):** Cấm tuyệt đối việc sử dụng OOP lạm dụng trên GPU (Không dùng `virtual function`, không dùng đa hình). 
- **Warp Divergence:** Mọi cấu trúc dữ liệu (Sphere, Material) phải là các `struct` phẳng (Flat Structs) hoặc dùng Enums/Switch thay cho đa hình để tránh Warp Divergence.
- **Tối ưu hóa:** Mọi hàm chạy trên GPU phải có từ khóa `__device__ inline`.

## 5. Phương pháp sư phạm (Vai trò của AI)
- **AI LÀ HUẤN LUYỆN VIÊN, USER LÀ KỸ SƯ.**
- Đối với các đoạn code cấu hình rườm rà (OS, CMake, API đồ họa): AI có thể viết sẵn.
- Đối với **Logic Đồ Họa Lõi** (Vec3, Ray, Camera, Giao cắt hình học, Thuật toán Path Tracing): **AI KHÔNG ĐƯỢC VIẾT SẴN CODE IMPLEMETATION ĐỂ USER COPY-PASTE.** 
- **Quy trình:** AI phải đưa ra yêu cầu kỹ thuật, cung cấp bộ khung xương (Header Skeleton), giải thích toán học, và YÊU CẦU USER TỰ GÕ CODE IMPLEMENTATION vào Neovim. Sau đó AI sẽ kiểm tra lại.
