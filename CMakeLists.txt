cmake_minimum_required(VERSION 3.20.0)
project(finis VERSION 0.1.0)

set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Assimp REQUIRED)
file(GLOB_RECURSE SRC src/*.cpp)

add_library(finis_base SHARED ${SRC})
target_include_directories(finis_base PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis_base PUBLIC glfw)
target_link_libraries(finis_base PUBLIC Vulkan::Vulkan)
target_link_libraries(finis_base PUBLIC assimp)
target_link_libraries(finis_base PUBLIC shaderc_shared)
TARGET_LINK_LIBRARIES(finis_base PUBLIC boost_filesystem-mt.dll boost_system-mt.dll)

add_library(finis_test SHARED test.c)
target_include_directories(finis_test PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis_test PUBLIC finis_base)

add_executable(finis main.cpp)
target_include_directories(finis PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis PUBLIC finis_base)

