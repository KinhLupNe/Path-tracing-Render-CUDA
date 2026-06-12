#include "gl_loader.h"
#include "geometry/hittable.h"
#include "math/vec3.h"
#include "render_kernel.h"
#include <GLFW/glfw3.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>
#include <iostream>

#include "core/ui_manager.h"
#include "core/camera_controller.h"

#include "core/scene.h"
#include "loaders/hardcoded_loader.h"
#include "loaders/mitsuba_loader.h"
#include "loaders/test_hardcore_loader.h"

// Cấu trúc lưu trữ các thành phần cầu nối giữa CUDA và OpenGL
struct CudaGLInterop {
  GLuint pbo; // Pixel Buffer Object: Cục bộ nhớ trung gian nằm trên VRAM
  GLuint tex; // Texture: Bức ảnh 2D của OpenGL dùng để hiển thị lên màn hình
  cudaGraphicsResource *cuda_pbo_resource; // Chìa khóa (Pointer) để CUDA có quyền ghi vào PBO
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int main() {
  std::cout << "[INFO] Khoi tao GLFW..." << std::endl;
  if (!glfwInit())
    return -1;

  int width = 1950;
  int height = 1002;

  int w_width = 1950;
  int w_height = 1002;

  // Tạo cái vỏ cửa sổ (Window Frame) bằng GLFW
  GLFWwindow *window = glfwCreateWindow(w_width, w_height, "CUDA Path Tracer ", NULL, NULL);
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
  UIManager ui_manager;
  ui_manager.init(window);

  // Nhowf CUDA cap phat VRAM va khoi tao luon cuRand ho cpu
  curandState *d_rand_state;
  allocate_and_init_curand(&d_rand_state, width, height);

  // cấp phát bộ nhớ trên gpu cho Accumlation Buffer
  Vec3 *d_accumlation_buffer;
  cudaMalloc((void **)&d_accumlation_buffer, width * height * sizeof(Vec3));

  Hittable **d_world;
  Hittable **d_list;
  Camera **d_camera;
  cudaMalloc((void **)&d_world, sizeof(Hittable *));
  cudaMalloc((void **)&d_list, MAX_HITTABLES * sizeof(Hittable *));
  cudaMalloc((void **)&d_camera, sizeof(Camera *));

  float aspect_ratio = float(width) / float(height);

  SceneLoader *loader = new HardcodedLoader();
  Scene scene = loader->load(aspect_ratio);
  delete loader;

  // except
  if (scene.spheres.empty()) {
    std::cout << "[INFO] Scene trong rỗng (hoặc không chứa khối cầu). Chuyển về chế độ Hardcoded "
                 "ngẫu nhiên!"
              << std::endl;
    loader = new HardcodedLoader();
    scene = loader->load(aspect_ratio);
    delete loader;
  }

  CameraController camera_controller(scene);

  SphereData *d_spheres;
  int num_spheres = scene.spheres.size();
  cudaMalloc((void **)&d_spheres, num_spheres * sizeof(SphereData));
  cudaMemcpy(d_spheres, scene.spheres.data(), num_spheres * sizeof(SphereData),
             cudaMemcpyHostToDevice);

  create_world_wrapper(d_list, d_world, d_camera, d_spheres, num_spheres, scene.camera);

  // Không cần giữ d_spheres trên VRAM nữa sau khi đã setup xong world (đã copy vào d_list bằng
  // `new`)
  cudaFree(d_spheres);

  int frame_count = 1;
  // Point3 prev_sphere_pos = sphere_pos;

  float time = 0.0f;
  float timet = time;
  bool flag = true;
  // Khoi tao state tren gpu
  std::cout << "[INFO] Bat dau Real-time Render Loop!" << std::endl;

  // VÒNG LẶP RENDER CHÍNH (Chạy liên tục 60 khung hình/giây)
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents(); // Lắng nghe chuột/bàn phím
    // ==========================================
    // BƯỚC 1: Xây dựng giao diện UI & Cập nhật Camera
    // ==========================================
    float current_time = glfwGetTime();
    static float last_time = current_time;
    float delta_time = current_time - last_time + 0.0001f;
    float framerate = 1.0f / delta_time;
    last_time = current_time;

    bool ui_changed = ui_manager.render_panel(scene, frame_count, framerate);
    bool cam_moved = camera_controller.update(window, scene, delta_time);

    if (ui_changed || cam_moved) {
      update_camera_wrapper(d_camera, scene.camera);
      frame_count = 1; // Reset accumulation vì camera vừa di chuyển
    }

    // ==========================================
    // BƯỚC 2: Giao quyền PBO cho CUDA xử lý (Tô màu)
    // ==========================================
    uchar4 *d_out; // Con trỏ mảng pixel trên GPU
    size_t num_bytes;

    // Khóa (Map) PBO lại, ép OpenGL không được đụng vào để CUDA làm việc
    cudaGraphicsMapResources(1, &interop.cuda_pbo_resource, 0);
    // Lấy con trỏ vật lý trỏ thẳng vào VRAM của PBO
    cudaGraphicsResourceGetMappedPointer((void **)&d_out, &num_bytes, interop.cuda_pbo_resource);

    // update_world_wrapper(d_list, sphere_pos, time);
    // Gọi hàng vạn luồng GPU nhảy vào tính toán màu sắc và ghi vào con trỏ
    launch_render_kernel(width, height, d_out, d_accumlation_buffer, d_rand_state, frame_count,
                         d_world, d_camera, time);
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
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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
    ui_manager.render_draw_data();

    // Đẩy khung hình ra màn hình (Double Buffering)
    glfwSwapBuffers(window);
  }

  // Dọn dẹp RAM/VRAM khi tắt ứng dụng
  ui_manager.shutdown();
  // Giải phóng các object trong VRAM trước, sau đó giải phóng con trỏ quản lên
  free_world_wrapper(d_list, d_world, d_camera);
  cudaFree(d_list);
  cudaFree(d_world);
  cudaFree(d_camera);

  cudaFree(d_accumlation_buffer);
  cudaFree(d_rand_state);

  cudaGraphicsUnregisterResource(interop.cuda_pbo_resource);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
