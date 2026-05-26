cmake_minimum_required(VERSION 3.18)

# Định nghĩa dự án tên là ScratchPathTracerCUDA, hỗ trợ 3 ngôn ngữ: C++, C và đặc biệt là CUDA
project(ScratchPathTracerCUDA LANGUAGES CXX C CUDA)

# Yêu cầu CMake sinh ra file compile_commands.json. 
# File này là "từ điển" để các trình biên tập như Neovim (thông qua clangd) đọc hiểu code C++ và CUDA
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ép trình biên dịch dùng chuẩn C++17 (chuẩn hiện đại, hỗ trợ nhiều tính năng mạnh mẽ)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cờ biên dịch đặc biệt của CUDA: -use_fast_math
# Ép GPU sử dụng các lệnh toán học xấp xỉ phần cứng (FMA, sin/cos xấp xỉ) để chạy cực nhanh thay vì tính chính xác tuyệt đối
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -use_fast_math")

# Thư viện FetchContent của CMake giúp tự động tải mã nguồn từ Github về mà không cần tải thủ công bằng tay
include(FetchContent)

# 1. TẢI VÀ TÍCH HỢP GLFW (Thư viện tạo Cửa Sổ và nhận tín hiệu Chuột/Phím)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# 2. TẢI VÀ TÍCH HỢP DEAR IMGUI (Thư viện vẽ Giao Diện Điều Khiển UI cực nhẹ)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Gom toàn bộ mã nguồn của ImGui vào một biến để lát nữa đem đi biên dịch chung với code của chúng ta
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    # Dùng backend GLFW và OpenGL2 Legacy để không cần cài cắm thư viện đồ họa (GLEW/GLAD) rườm rà
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)

# Thêm thư mục chứa file .h của ImGui vào đường dẫn Include để C++ tìm thấy
include_directories(
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Khai báo file chạy chính của chúng ta, bao gồm 4 file tự viết và toàn bộ mã nguồn ImGui
add_executable(ScratchPathTracerCUDA
    src/main.cpp
    src/gl_loader.cpp
    src/render_kernel.cu
    ${IMGUI_SOURCES}
)

# Cấu hình properties đặc biệt cho file .cu để nó biết đường tìm các file .h của C++ và ImGui
set_target_properties(ScratchPathTracerCUDA PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_include_directories(ScratchPathTracerCUDA PRIVATE src ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

# Cuối cùng, nói cho CMake biết phải liên kết (link) chương trình của chúng ta với thư viện GLFW và OpenGL của Windows
target_link_libraries(ScratchPathTracerCUDA PRIVATE glfw opengl32)
