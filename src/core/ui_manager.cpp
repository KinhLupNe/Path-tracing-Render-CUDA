#include "ui_manager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

void UIManager::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();
}

bool UIManager::render_panel(Scene& scene, int& frame_count, float framerate) {
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool camera_changed = false;

    ImGui::Begin("Control Panel");

    ImGui::Text("Render Stats:");
    ImGui::Text("So khung hinh tich luy : %d", frame_count);
    ImGui::Text("Hieu nang: %.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);
    
    if (ImGui::Button("Reset Accumulation")) {
        frame_count = 1;
    }
    
    static bool always_reset = false;
    ImGui::Checkbox("Reset moi frame (1 spp)", &always_reset);
    if (always_reset) {
        frame_count = 1;
    }

    ImGui::Separator();
    ImGui::Text("Camera Settings:");

    static bool first_time = true;

    // Đồng bộ thông số UI với scene lần đầu tiên
    if (first_time) {
        scene.current_aperture = scene.camera.lens_radius * 2.0f;
        // fov và focus_dist dùng mặc định hoặc giá trị cũ vì khó extract ngược
        first_time = false;
    }

    if (ImGui::SliderFloat("FOV", &scene.current_fov, 1.0f, 120.0f)) camera_changed = true;
    if (ImGui::SliderFloat("Aperture", &scene.current_aperture, 0.0f, 0.3f)) camera_changed = true;
    if (ImGui::DragFloat("Focus Dist", &scene.current_focus_dist, 0.1f, 0.1f, 100.0f)) camera_changed = true;

    if (camera_changed) {
        float aspect_ratio = scene.camera.horizontal.length() / scene.camera.vertical.length();
        Point3 lookfrom = scene.camera.origin;
        Point3 lookat = lookfrom - scene.camera.w;
        // Cập nhật lại đối tượng Camera trên RAM
        scene.set_camera(Camera(lookfrom, lookat, Vec3(0,1,0), scene.current_fov, aspect_ratio, scene.current_aperture, scene.current_focus_dist));
    }

    ImGui::End();

    ImGui::Render();

    return camera_changed;
}

void UIManager::render_draw_data() {
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::shutdown() {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
