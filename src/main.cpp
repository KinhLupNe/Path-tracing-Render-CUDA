#include "gl_loader.h"
#include "math/vec3.h"
#include "render_kernel.h"
#include <GLFW/glfw3.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>
#include <iostream>

// Thư viện ImGui dùng để tạo giao diện điều khiển
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

// Cấu trúc lưu trữ các thành phần cầu nối giữa CUDA và OpenGL
struct CudaGLInterop {
  GLuint pbo; // Pixel Buffer Object: Cục bộ nhớ trung gian nằm trên VRAM
  GLuint tex; // Texture: Bức ảnh 2D của OpenGL dùng để hiển thị lên màn hình
  cudaGraphicsResource
      *cuda_pbo_resource; // Chìa khóa (Pointer) để CUDA có quyền ghi vào PBO
  int width, height;
};

// Hàm khởi tạo cầu nối Zero-Copy giữa CUDA và OpenGL
void init_interop(CudaGLInterop &interop, int w, int h) {
  interop.width = w;
  interop.height = h;

  // 1. OPENGL: Tạo một bộ đệm PBO (Cái thớt chung) rỗng, đủ sức chứa w * h
  // pixels (mỗi pixel 4 bytes RGBA)
  glGenBuffers(1, &interop.pbo);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, interop.pbo);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, w * h * 4, nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // 2. CUDA-OPENGL INTEROP: Đăng ký PBO này với CUDA.
  // Từ giờ trở đi, CUDA có thể xin quyền ghi thẳng dữ liệu vào PBO này.
  cudaGraphicsGLRegisterBuffer(&interop.cuda_pbo_resource, interop.pbo,
                               cudaGraphicsMapFlagsWriteDiscard);

  // 3. OPENGL: Tạo một Texture 2D (Bức tranh trống) để lát nữa sẽ dán PBO vào
  // đây
  glGenTextures(1, &interop.tex);
  glBindTexture(GL_TEXTURE_2D, interop.tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int main() {
  std::cout << "[INFO] Khoi tao GLFW..." << std::endl;
  if (!glfwInit())
    return -1;

  int width = 800;
  int height = 600;

  int w_width = 1032;
  int w_height = 774;

  // Tạo cái vỏ cửa sổ (Window Frame) bằng GLFW
  GLFWwindow *window =
      glfwCreateWindow(w_width, w_height, "CUDA Path Tracer ", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Bật V-Sync để chống xé hình

  // Load các hàm mờ nhạt của OpenGL (như glGenBuffers)
  if (!load_gl_functions()) {
    std::cerr << "[ERROR] Khong the load ham OpenGL PBO!" << std::endl;
    return -1;
  }

  CudaGLInterop interop;
  init_interop(interop, width, height);

  // KHỞI TẠO IMGUI (UI)
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL2_Init();

  Point3 sphere_pos(0.0f, 0.0f, -1.0f);
  float sphere_radius = 0.5f;

  // Nhowf CUDA cap phat VRAM va khoi tao luon cuRand ho cpu
  curandState *d_rand_state;
  allocate_and_init_curand(&d_rand_state, width, height);

  // cấp phát bộ nhớ trên gpu cho Accumlation Buffer
  Vec3 *d_accumlation_buffer;
  cudaMalloc((void **)&d_accumlation_buffer, width * height * sizeof(Vec3));

  int frame_count = 1;
  Point3 prev_sphere_pos = sphere_pos;
  float prev_sphere_radius = sphere_radius;

  // Khoi tao state tren gpu
  std::cout << "[INFO] Bat dau Real-time Render Loop!" << std::endl;

  // VÒNG LẶP RENDER CHÍNH (Chạy liên tục 60 khung hình/giây)
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents(); // Lắng nghe chuột/bàn phím
    // ==========================================
    // BƯỚC 1: Xây dựng giao diện ImGui
    // ==========================================
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Control:");
    static bool animate = false;
    ImGui::Checkbox("accum, ana off ", &animate);

    // Neu co bat ki su thay doi thong so nao, hoawc dang bat animate, ta se ep
    // frame count ve 1 de xoa bo anh cu
    if (ImGui::Button("Reset Accumlation ") || animate ||
        prev_sphere_pos.e[0] != sphere_pos.e[0] ||
        prev_sphere_pos.e[1] != sphere_pos.e[1] ||
        prev_sphere_pos.e[2] != sphere_pos.e[2] ||
        prev_sphere_radius != sphere_radius) {
      frame_count = 1;
      prev_sphere_pos = sphere_pos;
      prev_sphere_radius = sphere_radius;
    }
    // Neu bat anime thi thoi gian troi, con khong thi dong bang
    float time = animate ? glfwGetTime() : 0.0f;
    ImGui::Text("So khung hinh tich luy : %d", frame_count);
    // ImGui::Text("Chao mung den voi CUDA Path Tracer!");
    ImGui::Separator();
    ImGui::SliderFloat3("X, Y, Z", sphere_pos.e, -5.0f, 5.0f);
    ImGui::SliderFloat("Radius", &sphere_radius, 0.1f, 3.0f);
    ImGui::Text("Hieu nang: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate,
                io.Framerate);
    if (ImGui::Button("Thoat")) {
      glfwSetWindowShouldClose(window, true);
    }
    ImGui::End();

    // ==========================================
    // BƯỚC 2: Giao quyền PBO cho CUDA xử lý (Tô màu)
    // ==========================================
    uchar4 *d_out; // Con trỏ mảng pixel trên GPU
    size_t num_bytes;

    // Khóa (Map) PBO lại, ép OpenGL không được đụng vào để CUDA làm việc
    cudaGraphicsMapResources(1, &interop.cuda_pbo_resource, 0);
    // Lấy con trỏ vật lý trỏ thẳng vào VRAM của PBO
    cudaGraphicsResourceGetMappedPointer((void **)&d_out, &num_bytes,
                                         interop.cuda_pbo_resource);

    // Gọi hàng vạn luồng GPU nhảy vào tính toán màu sắc và ghi vào con trỏ
    // d_out
    launch_render_kernel(width, height, d_out, d_accumlation_buffer,
                         d_rand_state, frame_count, sphere_pos, sphere_radius,
                         time);
    frame_count++;

    // CUDA xong việc, nhả khóa (Unmap) trả PBO lại cho OpenGL
    cudaGraphicsUnmapResources(1, &interop.cuda_pbo_resource, 0);

    // ==========================================
    // BƯỚC 3: OpenGL lấy PBO dán lên màn hình
    // ==========================================
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, interop.pbo);
    glBindTexture(GL_TEXTURE_2D, interop.tex);
    // Copy dữ liệu siêu tốc bằng phần cứng DMA từ PBO (vừa được CUDA tô màu)
    // vào Texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                    GL_UNSIGNED_BYTE, nullptr);

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    // Vẽ một hình chữ nhật 2D (Quad) to bằng cửa sổ và dán cái Texture lên đó
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ==========================================
    // BƯỚC 4: Vẽ đè giao diện UI lên màn hình
    // ==========================================
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    // Đẩy khung hình ra màn hình (Double Buffering)
    glfwSwapBuffers(window);
  }

  // Dọn dẹp RAM/VRAM khi tắt ứng dụng
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  cudaGraphicsUnregisterResource(interop.cuda_pbo_resource);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
