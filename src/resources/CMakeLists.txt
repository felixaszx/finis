set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exe")

add_library(fastgltf STATIC IMPORTED)
set_target_properties(fastgltf PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/ext_libs/libfastgltf.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/fastgltf")

file(GLOB_RECURSE SRC "*.cpp")
add_library(finis_resources SHARED ${SRC})
target_include_directories(finis_resources PRIVATE "${CMAKE_SOURCE_DIR}/include")

find_package(simdjson)
target_link_libraries(finis_resources fastgltf)
target_link_libraries(finis_resources simdjson)
