#include <iostream>
#include <GLFW/glfw3.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include "gl_loader.h"
#include "render_kernel.h"

// Thư viện ImGui dùng để tạo giao diện điều khiển
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

// Cấu trúc lưu trữ các thành phần cầu nối giữa CUDA và OpenGL
struct CudaGLInterop {
    GLuint pbo; // Pixel Buffer Object: Cục bộ nhớ trung gian nằm trên VRAM
    GLuint tex; // Texture: Bức ảnh 2D của OpenGL dùng để hiển thị lên màn hình
    cudaGraphicsResource* cuda_pbo_resource; // Chìa khóa (Pointer) để CUDA có quyền ghi vào PBO
    int width, height;
};

// Hàm khởi tạo cầu nối Zero-Copy giữa CUDA và OpenGL
void init_interop(CudaGLInterop& interop, int w, int h) {
    interop.width = w; interop.height = h;
    
    // 1. OPENGL: Tạo một bộ đệm PBO (Cái thớt chung) rỗng, đủ sức chứa w * h pixels (mỗi pixel 4 bytes RGBA)
    glGenBuffers(1, &interop.pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, interop.pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, w * h * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // 2. CUDA-OPENGL INTEROP: Đăng ký PBO này với CUDA. 
    // Từ giờ trở đi, CUDA có thể xin quyền ghi thẳng dữ liệu vào PBO này.
    cudaGraphicsGLRegisterBuffer(&interop.cuda_pbo_resource, interop.pbo, cudaGraphicsMapFlagsWriteDiscard);

    // 3. OPENGL: Tạo một Texture 2D (Bức tranh trống) để lát nữa sẽ dán PBO vào đây
    glGenTextures(1, &interop.tex);
    glBindTexture(GL_TEXTURE_2D, interop.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int main() {
    std::cout << "[INFO] Khoi tao GLFW..." << std::endl;
    if (!glfwInit()) return -1;

    int width = 800;
    int height = 600;

    // Tạo cái vỏ cửa sổ (Window Frame) bằng GLFW
    GLFWwindow* window = glfwCreateWindow(width, height, "CUDA Path Tracer + ImGui", NULL, NULL);
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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); 
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    float time = 0.0f;
    float anim_speed = 10.0f; 

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

        ImGui::Begin("Bang Dieu Khien GPU");
        ImGui::Text("Chao mung den voi CUDA Path Tracer!");
        ImGui::Separator();
        ImGui::SliderFloat("Toc do gon song", &anim_speed, 1.0f, 50.0f);
        ImGui::Text("Hieu nang: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        if (ImGui::Button("Thoat Chuong Trinh")) {
            glfwSetWindowShouldClose(window, true);
        }
        ImGui::End();

        // ==========================================
        // BƯỚC 2: Giao quyền PBO cho CUDA xử lý (Tô màu)
        // ==========================================
        uchar4* d_out; // Con trỏ mảng pixel trên GPU
        size_t num_bytes;
        
        // Khóa (Map) PBO lại, ép OpenGL không được đụng vào để CUDA làm việc
        cudaGraphicsMapResources(1, &interop.cuda_pbo_resource, 0);
        // Lấy con trỏ vật lý trỏ thẳng vào VRAM của PBO
        cudaGraphicsResourceGetMappedPointer((void**)&d_out, &num_bytes, interop.cuda_pbo_resource);

        // Gọi hàng vạn luồng GPU nhảy vào tính toán màu sắc và ghi vào con trỏ d_out
        launch_render_kernel(width, height, d_out, time, anim_speed); 

        // CUDA xong việc, nhả khóa (Unmap) trả PBO lại cho OpenGL
        cudaGraphicsUnmapResources(1, &interop.cuda_pbo_resource, 0);

        // ==========================================
        // BƯỚC 3: OpenGL lấy PBO dán lên màn hình
        // ==========================================
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, interop.pbo);
        glBindTexture(GL_TEXTURE_2D, interop.tex);
        // Copy dữ liệu siêu tốc bằng phần cứng DMA từ PBO (vừa được CUDA tô màu) vào Texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        // Vẽ một hình chữ nhật 2D (Quad) to bằng cửa sổ và dán cái Texture lên đó
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
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
        time += 0.01f;
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
