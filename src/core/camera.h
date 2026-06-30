#pragma once
#include "../math/vec3.h"
#include "../math/ray.h"
#include <curand_kernel.h>
#include <cmath>

class Camera {
public:
    Point3 origin;
    Point3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w;
    float lens_radius;
    float time0, time1;

    __device__ __host__ Camera() {}

    __device__ __host__ Camera(
        Point3 lookfrom,
        Point3 lookat,
        Vec3   vup,
        float vfov, // vertical field-of-view in degrees
        float aspect_ratio,
        float aperture,
        float focus_dist,
        float _time0 = 0.0f,
        float _time1 = 1.0f
    ) {
        float theta = vfov * (3.14159265f / 180.0f);
        float h = tan(theta / 2.0f);
        float viewport_height = 2.0f * h;
        float viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        origin = lookfrom;
        horizontal = focus_dist * viewport_width * u;
        vertical = focus_dist * viewport_height * v;
        lower_left_corner = origin - horizontal / 2.0f - vertical / 2.0f - focus_dist * w;

        lens_radius = aperture / 2.0f;
        time0 = _time0;
        time1 = _time1;
    }

    __device__ Ray get_ray(float s, float t, curandState *local_rand_state) const {
        Vec3 rd = lens_radius * random_in_unit_disk(local_rand_state);
        Vec3 offset = u * rd.x() + v * rd.y();
        
        float time = time0 + random_float(local_rand_state) * (time1 - time0);

        return Ray(
            origin + offset,
            lower_left_corner + s * horizontal + t * vertical - origin - offset,
            time
        );
    }

    // He so giam sang toi goc theo dinh luat cos^4 (natural vignetting).
    // theta la goc giua tia va truc quang hoc (huong nhin la -w). Day la thua so
    // von bi loai bo khi chuan hoa do do camera; giu lai de tai tao hieu ung
    // toi dan ve phia bien anh nhu mot may anh that.
    __device__ float vignette_cos4(const Ray &r) const {
        float cos_theta = dot(unit_vector(r.direction()), -w);
        if (cos_theta <= 0.0f) return 0.0f;
        float c2 = cos_theta * cos_theta;
        return c2 * c2; // cos^4(theta)
    }
};
