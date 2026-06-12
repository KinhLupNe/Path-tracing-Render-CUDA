#pragma once
#include "core/scene.h"
#include "scene_loader.h"

class TestHardcoreLoader : public SceneLoader {
public:
    Scene load(float aspect_ratio) override {
        Scene scene;

        // Camera setup - Nhìn ngang tầm mắt vào 3 quả cầu chính
        Point3 lookfrom(0, 2.0, 8.0);
        Point3 lookat(0, 1.0, 0);
        Vec3 vup(0, 1, 0);
        float dist_to_focus = (lookfrom - lookat).length();
        float aperture = 0.0f;
        float vfov = 40.0f;
        scene.set_camera(Camera(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus));

        // 1. Mặt đất - Lambertian xám
        scene.add_sphere(Point3(0.0f, -1000.0f, 0.0f), 1000.0f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.5f, 0.5f, 0.5f), 0, 0));

        // 2. Ba quả cầu lớn - mỗi quả một loại vật liệu
        // Trái: Lambertian (khuếch tán) màu đỏ gạch
        scene.add_sphere(Point3(-2.2f, 1.0f, 0.0f), 1.0f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.7f, 0.3f, 0.3f), 0, 0));

        // Giữa: Dielectric (thủy tinh), chiết suất 1.5
        scene.add_sphere(Point3(0.0f, 1.0f, 0.0f), 1.0f,
                         Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f));

        // Phải: Metal (kim loại) màu vàng đồng, hơi nhám
        scene.add_sphere(Point3(2.2f, 1.0f, 0.0f), 1.0f,
                         Material(MaterialType::METAL, Vec3(0.8f, 0.6f, 0.2f), 0.1f, 0));

        // 3. Hàng cầu nhỏ phía trước - xen kẽ 3 loại vật liệu (tất cả đều đứng yên)
        float small_radius = 0.3f;
        for (int i = -3; i <= 3; i++) {
            Point3 center(i * 1.1f, small_radius, 2.5f);
            Material mat;
            int type = (i + 3) % 3;
            if (type == 0) {
                // Lambertian màu thay đổi theo vị trí
                Vec3 albedo(0.3f + 0.1f * (i + 3), 0.4f, 0.8f - 0.1f * (i + 3));
                mat = Material(MaterialType::LAMBERTIAN, albedo, 0, 0);
            } else if (type == 1) {
                // Metal bạc, độ nhám tăng dần
                mat = Material(MaterialType::METAL, Vec3(0.9f, 0.9f, 0.9f), 0.05f * (i + 3), 0);
            } else {
                // Dielectric thủy tinh
                mat = Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f);
            }
            scene.add_sphere(center, small_radius, mat);
        }

        return scene;
    }
};
