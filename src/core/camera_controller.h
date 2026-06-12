#pragma once
#include "core/scene.h"
#include <GLFW/glfw3.h>

class CameraController {
public:
    CameraController(Scene& scene);
    
    // Xử lý input từ bàn phím và chuột. Trả về true nếu camera bị di chuyển
    bool update(GLFWwindow* window, Scene& scene, float delta_time);

private:
    float pitch;
    float yaw;
    Point3 position;
    
    float movement_speed = 10.0f;
    float mouse_sensitivity = 0.1f;
    
    bool first_mouse = true;
    float last_x = 0.0f;
    float last_y = 0.0f;
};
