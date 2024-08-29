cmake_minimum_required(VERSION 3.20.0)
project(finis VERSION 0.1.0)

set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

add_subdirectory("src/graphics")
add_subdirectory("src/extensions")
add_subdirectory("ext_dls")

file(GLOB_RECURSE SRC app/*.cpp )
add_executable(finis ${SRC})
target_include_directories(finis PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis PRIVATE fltk)

target_link_libraries(finis PRIVATE finis_extension)
target_link_libraries(finis PRIVATE finis_graphics)

add_executable(fltk_test fltk.cpp)
target_include_directories(fltk_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(fltk_test PRIVATE fltk)
