cmake_minimum_required(VERSION 3.20)
project(OptiScalerMgrLite LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 -D_CRT_SECURE_NO_WARNINGS)

file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp" "src/*.rc" "src/*.h")
add_executable(${PROJECT_NAME} WIN32 ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

target_link_libraries(${PROJECT_NAME} PRIVATE
  comctl32 Windowscodecs Shell32 Shlwapi Winhttp Version Ole32 uuid d3d12 d3d11 dxgi d2d1 dwrite)

if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W3 /permissive- /Zc:__cplusplus /utf-8)
endif()
