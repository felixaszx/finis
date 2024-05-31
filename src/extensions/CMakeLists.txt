set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_C_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

file(GLOB_RECURSE EXTENSION_SRC *.cpp)
add_library(finis_extension SHARED ${EXTENSION_SRC})
target_include_directories(finis_extension PUBLIC ${CMAKE_SOURCE_DIR}/include)
TARGET_LINK_LIBRARIES(finis_extension PUBLIC boost_filesystem-mt.dll boost_system-mt.dll)