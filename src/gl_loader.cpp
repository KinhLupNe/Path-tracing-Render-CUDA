#include "gl_loader.h"

// Biến lưu trữ địa chỉ của các hàm OpenGL. Ban đầu gán bằng nullptr (rỗng).
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;

// Kỹ thuật "Khủng long": Lấy hàm từ Driver đồ họa trên hệ điều hành Windows
bool load_gl_functions() {
    // wglGetProcAddress là một hàm đặc biệt của Windows API, dùng để xin Driver cấp cho địa chỉ 
    // của bất kỳ hàm OpenGL hiện đại nào không có sẵn trong thư viện chuẩn.
    
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");

    // Nếu lấy thành công cả 3 hàm, trả về true. Nếu bị lỗi (VGA không hỗ trợ), trả về false.
    return (glGenBuffers && glBindBuffer && glBufferData);
}
