set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exe")

file(GLOB_RECURSE SRC "*.c" "*.cpp")
add_library(finis_lib SHARED ${SRC})
target_include_directories(finis_lib PRIVATE "${CMAKE_SOURCE_DIR}/include")

find_package(PkgConfig REQUIRED)

find_package(glfw3 REQUIRED)
find_package(spirv_cross_reflect REQUIRED)
find_package(spirv_cross_core REQUIRED)

target_link_libraries(finis_lib PUBLIC glfw)