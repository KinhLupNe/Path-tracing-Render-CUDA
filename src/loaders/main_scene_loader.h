#pragma once
#include "core/scene.h"
#include "scene_loader.h"
#include <cmath>

// Khung cảnh chính: phong cách "Ray Tracing in One Weekend" final scene,
// nhưng toàn bộ vật thể đều đứng yên (không có moving sphere).
class MainSceneLoader : public SceneLoader {
private:
    // LCG tự viết với seed cố định để khung cảnh luôn giống nhau mỗi lần chạy
    unsigned int rng_state = 42u;

    float random_float() {
        rng_state = rng_state * 1664525u + 1013904223u;
        return (rng_state >> 8) / 16777216.0f; // [0, 1)
    }

    float random_float(float min, float max) {
        return min + (max - min) * random_float();
    }

public:
    Scene load(float aspect_ratio) override {
        Scene scene;

        // Camera: Nhìn từ trên cao chéo xuống để thấy rõ cấu trúc xoắn ốc
        // Đẩy camera ra xa (25, 12, 25) để tránh bị kẹt vào các quả cầu ở rìa ngoài của vòng xoắn
        Point3 lookfrom(25.0f, 12.0f, 25.0f);
        Point3 lookat(0.0f, 0.0f, 0.0f);
        Vec3 vup(0, 1, 0);
        float dist_to_focus = 37.0f; // Khoảng cách từ (25, 12, 25) đến gốc tọa độ
        float aperture = 0.02f; // Khẩu độ nhỏ để rõ nét toàn bộ cấu trúc
        float vfov = 20.0f; // Thu hẹp góc nhìn một chút vì camera đã ở xa
        scene.set_camera(Camera(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus));

        // Mặt đất: Lambertian màu cát ấm
        scene.add_sphere(Point3(0.0f, -1000.0f, 0.0f), 1000.0f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.48f, 0.46f, 0.42f), 0, 0));

        // 3 quả cầu chính
        Point3 hero_centers[3] = {
            Point3(0.0f, 1.0f, 0.0f),   // Thủy tinh ở giữa
            Point3(-4.0f, 1.0f, 0.0f),  // Lambertian xanh navy
            Point3(4.0f, 1.0f, 0.0f),   // Kim loại vàng champagne, bóng loáng
        };
        scene.add_sphere(hero_centers[0], 1.0f,
                         Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f));
        scene.add_sphere(hero_centers[1], 1.0f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.15f, 0.25f, 0.55f), 0, 0));
        scene.add_sphere(hero_centers[2], 1.0f,
                         Material(MaterialType::METAL, Vec3(0.85f, 0.70f, 0.45f), 0.0f, 0));

        // Bảng màu pastel cho các cầu nhỏ Lambertian
        const Vec3 palette[6] = {
            Vec3(0.90f, 0.45f, 0.45f), // hồng san hô
            Vec3(0.95f, 0.75f, 0.35f), // vàng nghệ
            Vec3(0.45f, 0.75f, 0.50f), // xanh bạc hà
            Vec3(0.40f, 0.60f, 0.85f), // xanh da trời
            Vec3(0.70f, 0.50f, 0.80f), // tím oải hương
            Vec3(0.90f, 0.90f, 0.85f), // trắng ngà
        };

        // Cấu trúc Xoắn ốc Fibonacci (Fibonacci Spiral) tạo vẻ đẹp toán học thần thánh
        int num_spheres = 600;
        float golden_angle = 137.507764f * 3.14159265f / 180.0f;
        float c = 0.55f; // Độ giãn của vòng xoắn

        for (int i = 1; i <= num_spheres; i++) {
            float r = c * sqrt((float)i);
            float theta = i * golden_angle;
            
            float x = r * cos(theta);
            float z = r * sin(theta);
            
            float radius = random_float(0.12f, 0.25f);
            Point3 center(x, radius, z);

            // Bỏ qua vị trí quá gần 3 quả cầu lớn
            bool too_close = false;
            for (int k = 0; k < 3; k++) {
                Vec3 d = center - hero_centers[k];
                if (d.x() * d.x() + d.z() * d.z() < 1.6f * 1.6f) {
                    too_close = true;
                    break;
                }
            }
            if (too_close) continue;

            float choose = random_float();
            Material m;
            if (choose < 0.60f) {
                // Lambertian pastel
                Vec3 albedo = palette[(int)(random_float() * 5.99f)];
                m = Material(MaterialType::LAMBERTIAN, albedo, 0, 0);
            } else if (choose < 0.85f) {
                // Kim loại ánh kim sáng
                Vec3 albedo(random_float(0.7f, 1.0f), random_float(0.7f, 1.0f),
                            random_float(0.7f, 1.0f));
                m = Material(MaterialType::METAL, albedo, random_float(0.0f, 0.15f), 0);
            } else {
                // Thủy tinh tinh khiết
                m = Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f);
            }

            scene.add_sphere(center, radius, m);
        }

        return scene;
    }
};
