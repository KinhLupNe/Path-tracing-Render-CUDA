#pragma once

// Trên Windows, các hàm OpenGL hiện đại (như glGenBuffers của PBO) không có sẵn trong file opengl32.dll gốc.
// Chúng ta phải "móc" chúng ra trực tiếp từ Card Màn Hình thông qua Driver (Extension).
// File này dùng để khai báo trước các con trỏ hàm đó để C++ không báo lỗi undefined.

#include <windows.h>
#include <GL/gl.h>

// Định nghĩa các hằng số của OpenGL liên quan đến PBO (Pixel Buffer Object)
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#define GL_DYNAMIC_DRAW        0x88E8
#define GL_ARRAY_BUFFER        0x8892

#include <cstddef>
typedef ptrdiff_t GLsizeiptr;

// Khai báo kiểu (typedef) cho các hàm OpenGL mà chúng ta cần
typedef void (WINAPI * PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (WINAPI * PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (WINAPI * PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);

// Khai báo biến toàn cục (extern) chứa con trỏ hàm, để các file khác (như main.cpp) có thể gọi được
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;

// Hàm sẽ được gọi đầu tiên trong main() để moi các hàm này từ Driver ra
bool load_gl_functions();
