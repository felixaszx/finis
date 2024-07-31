set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

find_package(glfw3 REQUIRED)

add_subdirectory(fastgltf)

file(GLOB_RECURSE SRC *.cpp )
add_library(finis_graphics2 SHARED ${SRC})
target_include_directories(finis_graphics2 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(finis_graphics2 PUBLIC fastgltf/include)

target_link_libraries(finis_graphics2 PRIVATE fastgltf)
target_link_libraries(finis_graphics2 PUBLIC glfw)
target_link_libraries(finis_graphics2 PUBLIC vulkan-1)
target_link_libraries(finis_graphics2 PUBLIC shaderc_shared)