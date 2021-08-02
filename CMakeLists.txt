cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
project(integral-image)

enable_testing()

find_package( OpenCV REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories(src)
include_directories(lib)

add_subdirectory(src)
