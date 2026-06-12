#pragma once
#include "core/scene.h"
#include <cstdlib>

#include "scene_loader.h"

class HardcodedLoader : public SceneLoader {
private:
    static inline float random_float() {
        return rand() / (RAND_MAX + 1.0f);
    }

    static inline float random_float(float min, float max) {
        return min + (max - min) * random_float();
    }

public:
    Scene load(float aspect_ratio) override {
        Scene scene;

        Point3 lookfrom(0, 0, 0);
        Point3 lookat(0, 0, -1);
        Vec3 vup(0, 1, 0);
        float dist_to_focus = 1.0;
        float aperture = 0.0;
        float vfov = 66.0375f;
        scene.set_camera(Camera(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus));

        scene.add_sphere(Point3(0.0f, -1000.5f, -1.0f), 1000.0f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.5f, 0.5f, 0.5f), 0, 0));

        Point3 big_centers[3] = {
            Point3(-2.0f, 0.0f, -4.5f),
            Point3(0.0f, 0.0f, -5.0f), 
            Point3(2.0f, 0.0f, -4.5f), 
        };
        scene.add_sphere(big_centers[0], 0.5f,
                         Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f));
        scene.add_sphere(big_centers[1], 0.5f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.4f, 0.2f, 0.1f), 0, 0));
        scene.add_sphere(big_centers[2], 0.5f,
                         Material(MaterialType::METAL, Vec3(0.7f, 0.6f, 0.5f), 0.0f, 0));

        scene.add_sphere(Point3(-0.78f, 0.12f, -1.45f), 0.62f,
                         Material(MaterialType::LAMBERTIAN, Vec3(0.8f, 0.2f, 0.2f), 0, 0));
        scene.add_sphere(Point3(0.0f, 0.85f, -2.40f), 0.62f,
                         Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f));
        scene.add_sphere(Point3(0.78f, 0.12f, -1.45f), 0.60f,
                         Material(MaterialType::METAL, Vec3(0.8f, 0.8f, 0.9f), 0.10f, 0));

        for (int a = -4; a <= 4; a++) {
            for (int b = 0; b <= 8; b++) {
                Point3 center(a + 0.7f * random_float(), -0.3f,
                              -2.0f - 1.0f * b + 0.7f * random_float());

                bool too_close = false;
                for (int k = 0; k < 3; k++) {
                    Vec3 d = center - big_centers[k];
                    if (d.x() * d.x() + d.z() * d.z() < 0.9f * 0.9f) {
                        too_close = true;
                        break;
                    }
                }

                Point3 near_centers[3] = {
                    Point3(-0.78f, 0.12f, -1.45f), 
                    Point3(0.78f, 0.12f, -1.45f),  
                    Point3(0.0f, 0.85f, -2.40f),   
                };
                for (int k = 0; k < 3; k++) {
                    Vec3 d = center - near_centers[k];
                    if (d.x() * d.x() + d.z() * d.z() < 1.4f * 1.4f) {
                        too_close = true;
                        break;
                    }
                }

                if (too_close) continue;

                float choose = random_float();
                Material m;
                if (choose < 0.7f) {
                    Vec3 albedo(random_float() * random_float(), random_float() * random_float(),
                                random_float() * random_float());
                    m = Material(MaterialType::LAMBERTIAN, albedo, 0, 0);
                    Point3 center2 = center + Vec3(0, random_float(0.0f, 0.5f), 0);
                    scene.add_moving_sphere(center, center2, 0.0f, 1.0f, 0.2f, m);
                    continue;

                } else if (choose < 0.9f) {
                    Vec3 albedo(0.5f * (1.0f + random_float()), 0.5f * (1.0f + random_float()),
                                0.5f * (1.0f + random_float()));
                    m = Material(MaterialType::METAL, albedo, 0.5f * random_float(), 0);
                } else {
                    m = Material(MaterialType::DIELECTRIC, Vec3(1.0f, 1.0f, 1.0f), 0, 1.5f);
                }

                scene.add_sphere(center, 0.2f, m);
            }
        }
        return scene;
    }
};
