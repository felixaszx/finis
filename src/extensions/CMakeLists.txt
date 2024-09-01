set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exe")

file(GLOB_RECURSE EXTENSION_SRC "*.cpp")
add_library(finis_extensions SHARED ${EXTENSION_SRC})
target_include_directories(finis_extensions PUBLIC "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(finis_extensions PUBLIC boost_filesystem-mt.dll)
target_link_libraries(finis_extensions PUBLIC boost_system-mt.dll)