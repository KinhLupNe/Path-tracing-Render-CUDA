#pragma once
#include "core/scene.h"

// Interface chung cho tất cả các loại loader
class SceneLoader {
public:
    virtual ~SceneLoader() = default;
    
    // Hàm nạp scene, nhận vào tỉ lệ khung hình (width/height)
    virtual Scene load(float aspect_ratio) = 0;
};
