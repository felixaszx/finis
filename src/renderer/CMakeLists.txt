set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

file(GLOB_RECURSE SRC "*.c" "*.cpp")
add_library(finis_renderer SHARED ${SRC})
target_include_directories(finis_renderer PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(finis_renderer PRIVATE "${CMAKE_SOURCE_DIR}/include/renderer")

find_package(PkgConfig REQUIRED)

target_link_libraries(finis_renderer PUBLIC finis_gfx)