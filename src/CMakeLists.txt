set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

file(GLOB SRC "*.c" "*.cpp")
add_library(finis_common SHARED ${SRC})
target_include_directories(finis_common PRIVATE "${CMAKE_SOURCE_DIR}/include")

find_package(PkgConfig REQUIRED)
find_package(glfw3 REQUIRED)

if(WIN32)
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_DL_LIBS dlfcn-win32::dl)
endif()

target_link_libraries(finis_common PUBLIC ${CMAKE_DL_LIBS})