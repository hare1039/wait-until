cmake_minimum_required(VERSION 3.5.1)
set(HAVE_FLAG_SEARCH_PATHS_FIRST 0)

project(wait-until)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup()

add_executable(wait-until wait-until.cpp)
target_link_libraries(wait-until ${CONAN_LIBS})

IF ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
   target_link_libraries(run ws2_32 wsock32)
ENDIF ()
