#pragma once

#include <vector>
#include "geometry/sphere.h"
#include "core/camera.h"

// Struct chứa dữ liệu của một mặt cầu để lưu trên Host và gửi sang Device
struct SphereData {
    Point3 center;
    float radius;
    Material mat;

    // Các thuộc tính mở rộng cho Moving Sphere
    bool is_moving = false;
    Point3 center2;
    float time0 = 0.0f;
    float time1 = 0.0f;
};

// Lớp Scene quản lý dữ liệu trên CPU (Host)
class Scene {
public:
    std::vector<SphereData> spheres;
    Camera camera;

    float current_fov = 60.0f;
    float current_aperture = 0.0f;
    float current_focus_dist = 10.0f;

    // Cuong do vignette: 0 = tat, 1 = dung cos^4 vat ly, >1 = lam toi qua muc.
    float vignette_strength = 1.0f;

    Scene() {}

    void add_sphere(const Point3& center, float radius, const Material& mat) {
        SphereData data;
        data.center = center;
        data.radius = radius;
        data.mat = mat;
        data.is_moving = false;
        spheres.push_back(data);
    }

    void add_moving_sphere(const Point3& center0, const Point3& center1, float time0, float time1, float radius, const Material& mat) {
        SphereData data;
        data.center = center0;
        data.radius = radius;
        data.mat = mat;
        data.is_moving = true;
        data.center2 = center1;
        data.time0 = time0;
        data.time1 = time1;
        spheres.push_back(data);
    }

    void set_camera(const Camera& cam) {
        camera = cam;
    }
};
