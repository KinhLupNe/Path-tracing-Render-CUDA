#include "camera_controller.h"
#include <cmath>

CameraController::CameraController(Scene& scene) {
    position = scene.camera.origin;
    
    // scene.camera.w là hướng ngược lại của look_direction
    Vec3 direction = unit_vector(-scene.camera.w);
    
    pitch = asin(direction.y()) * (180.0f / 3.14159265f);
    yaw = atan2(direction.z(), direction.x()) * (180.0f / 3.14159265f);
}

bool CameraController::update(GLFWwindow* window, Scene& scene, float delta_time) {
    bool moved = false;
    
    // 1. Mouse Input
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        if (first_mouse) {
            last_x = xpos;
            last_y = ypos;
            first_mouse = false;
        }
        
        float xoffset = xpos - last_x;
        float yoffset = last_y - ypos; // reversed since y-coordinates go from bottom to top
        last_x = xpos;
        last_y = ypos;
        
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        moved = true;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        first_mouse = true;
    }
    
    // Tính toán front vector
    Vec3 front;
    front.e[0] = cos(yaw * (3.14159265f / 180.0f)) * cos(pitch * (3.14159265f / 180.0f));
    front.e[1] = sin(pitch * (3.14159265f / 180.0f));
    front.e[2] = sin(yaw * (3.14159265f / 180.0f)) * cos(pitch * (3.14159265f / 180.0f));
    front = unit_vector(front);
    
    Vec3 right = unit_vector(cross(front, Vec3(0, 1, 0)));
    Vec3 up = unit_vector(cross(right, front));
    
    float velocity = movement_speed * delta_time;
    
    // 2. Keyboard Input
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { position = position + front * velocity; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { position = position - front * velocity; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { position = position - right * velocity; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { position = position + right * velocity; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { position = position + up * velocity; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { position = position - up * velocity; moved = true; }
    
    if (moved) {
        float aspect_ratio = scene.camera.horizontal.length() / scene.camera.vertical.length();
        Point3 lookat = position + front;
        scene.set_camera(Camera(position, lookat, Vec3(0,1,0), scene.current_fov, aspect_ratio, scene.current_aperture, scene.current_focus_dist));
    }
    return moved;
}
