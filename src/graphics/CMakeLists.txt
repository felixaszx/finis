set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

find_package(glfw3 REQUIRED)
add_library(fastgltf STATIC IMPORTED)
add_library(slang STATIC IMPORTED)
add_library(slang-rt STATIC IMPORTED)
set_target_properties(fastgltf PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/ext_libs/libfastgltf.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/fastgltf")
set_target_properties(slang PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/ext_libs/slang.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/slang")
set_target_properties(slang-rt PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/ext_libs/slang-rt.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/slang")

file(GLOB_RECURSE SRC *.cpp)
add_library(finis_graphics SHARED ${SRC})
target_include_directories(finis_graphics PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(finis_graphics PUBLIC glfw)
target_link_libraries(finis_graphics PUBLIC vulkan-1)
target_link_libraries(finis_graphics PUBLIC slang)
target_link_libraries(finis_graphics PUBLIC slang-rt)
target_link_libraries(finis_graphics PUBLIC spirv-cross-reflect)
target_link_libraries(finis_graphics PUBLIC spirv-cross-core)
target_link_libraries(finis_graphics PUBLIC spirv-cross-glsl)
target_link_libraries(finis_graphics PUBLIC spirv-cross-hlsl)
target_link_libraries(finis_graphics PUBLIC spirv-cross-cpp)
target_link_libraries(finis_graphics PUBLIC spirv-cross-util)