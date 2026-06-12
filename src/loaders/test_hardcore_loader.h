#pragma once
#include "core/scene.h"
#include "scene_loader.h"

class TestHardcoreLoader : public SceneLoader {
public:
    Scene load(float aspect_ratio) override {
        Scene scene;

        // Camera setup - Góc nhìn từ trên cao, lùi ra xa để thấy toàn cảnh
        Point3 lookfrom(0, 6.0, 12.0);
        Point3 lookat(0, 1.0, 0);
        Vec3 vup(0, 1, 0);
        float dist_to_focus = (lookfrom - lookat).length();
        float aperture = 0.0f;
        float vfov = 40.0f;
        scene.set_camera(Camera(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus));

        // 1. Mặt đất (Đổi thành màu đen tuyệt đối theo ý bạn để tạo tương phản siêu gắt)
        scene.add_sphere(Point3(0.0f, -1000.5f, 0.0f), 1000.0f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.0f, 0.0f, 0.0f), 0, 0));

        // 2. Một quả cầu nằm dưới cùng để hứng bóng
        scene.add_sphere(Point3(0.0f, 0.0f, 0.0f), 0.5f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.8f, 0.8f, 0.8f), 0, 0));

        // 3. Lưới 4x4 quả cầu màu sắc (được nâng lên cao y=1.2)
        float radius = 0.4f;
        float spacing = 0.85f; 
        Point3 offset(-1.5f * spacing, 1.2f, -1.5f * spacing);

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                Point3 center = offset + Vec3(i * spacing, 0, j * spacing);
                Vec3 albedo(0.2f + 0.2f * i, 0.2f + 0.2f * j, 0.3f + 0.1f * (i+j));
                scene.add_sphere(center, radius, Material(MaterialType::LAMBERTIAN, albedo, 0, 0));
            }
        }

        // 4. "Lưới che chắn" (Blocking Grid) - Tạo thành một cái lồng (Mái, Trái, Phải, Sau)
        float roof_radius = 0.4f;
        float roof_spacing = 1.2f;
        Material black_mat(MaterialType::LAMBERTIAN, Vec3(0.0f, 0.0f, 0.0f), 0, 0);

        // Trần nhà (Mái lưới 5x5) ở y = 4.5
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                Point3 center(i * roof_spacing, 4.5f, j * roof_spacing);
                scene.add_sphere(center, roof_radius, black_mat);
            }
        }

        // Tường Trái (x = -2.4) và Tường Phải (x = 2.4)
        for (int y = 0; y < 3; y++) {
            float h = 0.9f + y * roof_spacing; // Các tầng y = 0.9, 2.1, 3.3
            for (int j = -2; j <= 2; j++) {
                scene.add_sphere(Point3(-2.0f * roof_spacing, h, j * roof_spacing), roof_radius, black_mat); // Trái
                scene.add_sphere(Point3(2.0f * roof_spacing, h, j * roof_spacing), roof_radius, black_mat);  // Phải
            }
        }

        // Tường Sau (z = -2.4), không vẽ lại 2 cột ở góc đã vẽ bởi tường trái/phải
        for (int y = 0; y < 3; y++) {
            float h = 0.9f + y * roof_spacing;
            for (int i = -1; i <= 1; i++) {
                scene.add_sphere(Point3(i * roof_spacing, h, -2.0f * roof_spacing), roof_radius, black_mat);
            }
        }

        return scene;
    }
};
