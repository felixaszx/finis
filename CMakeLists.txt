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


add_executable(finis main.cpp)
target_include_directories(finis PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(finis PUBLIC finis_base)

# DLLs
file(GLOB_RECURSE SCRIPTS exts/*.c)
foreach(SCRIPT_FILE ${SCRIPTS})
    get_filename_component(SCRIPT_NAME ${SCRIPT_FILE} NAME_WLE)
    add_library(${SCRIPT_NAME} SHARED ${SCRIPT_FILE})
    target_include_directories(${SCRIPT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${SCRIPT_NAME} PUBLIC finis_base)
endforeach()
