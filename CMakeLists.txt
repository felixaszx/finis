cmake_minimum_required(VERSION 3.20.0)
project(finis VERSION 0.1.0)

set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_subdirectory(ext_dls)
add_subdirectory(src/graphics)
add_subdirectory(src/extensions)

add_executable(finis main.cpp)
target_include_directories(finis PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis PUBLIC finis_graphics)
target_link_libraries(finis PUBLIC finis_extension)
