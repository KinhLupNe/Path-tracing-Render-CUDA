#pragma once

#include <GLFW/glfw3.h>
#include "core/scene.h"

// Quản lý giao diện ImGui và điều khiển Camera
class UIManager {
public:
    void init(GLFWwindow* window);
    
    // Xây dựng bảng UI. Trả về true nếu Camera vừa bị thay đổi thông số
    bool render_panel(Scene& scene, int& frame_count, float framerate);
    
    // Vẽ giao diện lên màn hình (Gọi ở cuối vòng lặp)
    void render_draw_data();
    
    void shutdown();
};
