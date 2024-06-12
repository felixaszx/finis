set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Assimp REQUIRED)

file(GLOB_RECURSE GRAPHICS_SRC *.cpp)
add_library(finis_graphics SHARED ${GRAPHICS_SRC})
target_include_directories(finis_graphics PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis_graphics PUBLIC glfw)
target_link_libraries(finis_graphics PUBLIC Vulkan::Vulkan)
target_link_libraries(finis_graphics PUBLIC shaderc_shared)
target_link_libraries(finis_graphics PUBLIC finis_extension)