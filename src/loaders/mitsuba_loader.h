#pragma once
#include "scene_loader.h"
#include <tinyxml2.h>
#include <iostream>
#include <string>
#include <map>
#include <sstream>

class MitsubaLoader : public SceneLoader {
private:
    std::string filepath;

    // Hàm tiện ích để tách chuỗi "0.5, 0.5, 0.5" thành Vec3
    Vec3 parse_rgb(const std::string& str) {
        std::stringstream ss(str);
        std::string item;
        float r=0, g=0, b=0;
        if(std::getline(ss, item, ',')) r = std::stof(item);
        if(std::getline(ss, item, ',')) g = std::stof(item);
        if(std::getline(ss, item, ',')) b = std::stof(item);
        return Vec3(r, g, b);
    }

public:
    MitsubaLoader(const std::string& path) : filepath(path) {}

    Scene load(float aspect_ratio) override {
        Scene scene;
        tinyxml2::XMLDocument doc;
        
        std::cout << "[INFO] Dang doc file XML Mitsuba: " << filepath << std::endl;
        
        if (doc.LoadFile(filepath.c_str()) != tinyxml2::XML_SUCCESS) {
            std::cerr << "[ERROR] Khong the mo file Mitsuba: " << filepath << std::endl;
            return scene; // Trả về scene rỗng
        }

        tinyxml2::XMLElement* root = doc.FirstChildElement("scene");
        if (!root) return scene;

        std::map<std::string, Material> bsdf_map;

        // Duyệt qua tất cả các thẻ con của <scene>
        for (tinyxml2::XMLElement* e = root->FirstChildElement(); e != nullptr; e = e->NextSiblingElement()) {
            std::string name = e->Name();

            if (name == "sensor") {
                // 1. PARSE CAMERA (Sensor)
                float fov = 35.0f; // Giá trị mặc định
                tinyxml2::XMLElement* fov_elem = e->FirstChildElement("float");
                if (fov_elem && std::string(fov_elem->Attribute("name")) == "fov") {
                    fov = fov_elem->FloatAttribute("value");
                }

                // Chú ý: Ở phiên bản thật, ta cần đọc thẻ <matrix> và nhân ma trận 4x4.
                // Ở đây ta tạm hardcode vị trí camera đối xứng, nhìn vào gốc toạ độ.
                Point3 lookfrom(0, 1.0f, 6.8f);
                Point3 lookat(0, 1.0f, 0);
                Vec3 vup(0, 1, 0);
                scene.set_camera(Camera(lookfrom, lookat, vup, fov, aspect_ratio, 0.0f, 6.8f));
                std::cout << "[INFO] Tim thay Camera! FOV: " << fov << std::endl;
            } 
            else if (name == "bsdf") {
                // 2. PARSE MATERIAL (BSDF)
                std::string id = e->Attribute("id") ? e->Attribute("id") : "";
                std::string type = e->Attribute("type");
                
                tinyxml2::XMLElement* inner = e;
                if (type == "twosided") {
                    inner = e->FirstChildElement("bsdf");
                    if (inner) type = inner->Attribute("type");
                }

                if (type == "diffuse") {
                    Vec3 color(0.8f, 0.8f, 0.8f); // Default
                    tinyxml2::XMLElement* rgb = inner->FirstChildElement("rgb");
                    if (rgb && rgb->Attribute("value")) {
                        color = parse_rgb(rgb->Attribute("value"));
                    }
                    bsdf_map[id] = Material(MaterialType::LAMBERTIAN, color, 0, 0);
                } else if (type == "dielectric") {
                    bsdf_map[id] = Material(MaterialType::DIELECTRIC, Vec3(1,1,1), 0, 1.5f);
                } else if (type == "conductor" || type == "roughconductor") {
                    bsdf_map[id] = Material(MaterialType::METAL, Vec3(0.8f, 0.8f, 0.8f), 0.1f, 0);
                }
            }
            else if (name == "shape") {
                // 3. PARSE HÌNH KHỐI (Shape)
                std::string type = e->Attribute("type");
                std::string id = e->Attribute("id") ? e->Attribute("id") : "Unknown";
                
                if (type == "sphere") {
                    // Mặc định sphere ở gốc toạ độ
                    Point3 center(0, 0, 0);
                    float radius = 1.0f;
                    
                    tinyxml2::XMLElement* p_radius = e->FirstChildElement("float");
                    if (p_radius && std::string(p_radius->Attribute("name")) == "radius") {
                        radius = p_radius->FloatAttribute("value");
                    }
                    
                    // Lấy điểm trung tâm từ ma trận transform nếu có
                    tinyxml2::XMLElement* transform = e->FirstChildElement("transform");
                    if(transform) {
                         tinyxml2::XMLElement* matrix = transform->FirstChildElement("matrix");
                         if (matrix && matrix->Attribute("value")) {
                             std::stringstream ss(matrix->Attribute("value"));
                             std::string item;
                             std::vector<float> mat_vals;
                             while(std::getline(ss, item, ' ')) {
                                 if(!item.empty()) mat_vals.push_back(std::stof(item));
                             }
                             if(mat_vals.size() == 16) {
                                 // Cột thứ 4 của ma trận 4x4 là T (Translate)
                                 center = Point3(mat_vals[3], mat_vals[7], mat_vals[11]);
                             }
                         }
                    }

                    // Tìm vật liệu tương ứng
                    Material mat(MaterialType::LAMBERTIAN, Vec3(0.5f, 0.5f, 0.5f), 0, 0);
                    tinyxml2::XMLElement* ref = e->FirstChildElement("ref");
                    if (ref && ref->Attribute("id")) {
                        std::string bsdf_id = ref->Attribute("id");
                        if (bsdf_map.find(bsdf_id) != bsdf_map.end()) {
                            mat = bsdf_map[bsdf_id];
                        }
                    }

                    scene.add_sphere(center, radius, mat);
                    std::cout << "[INFO] Them hinh cau: " << id << " tai " << center.x() << "," << center.y() << "," << center.z() << std::endl;
                } else {
                    std::cerr << "[WARNING] Path Tracer hien tai chi ho tro hinh cau (Sphere). Bo qua hinh khoi: " << type << " (ID: " << id << ")" << std::endl;
                }
            }
        }

        return scene;
    }
};
