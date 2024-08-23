set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SRC *.cpp )
add_library(finis_engine SHARED ${SRC})
target_include_directories(finis_engine PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis_engine PRIVATE finis_graphics finis_extension)