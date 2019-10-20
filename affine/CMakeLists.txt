cmake_minimum_required(VERSION 2.8)

project (affine)

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

add_executable (affine affine.cxx)

target_link_libraries (affine ${ITK_LIBRARIES})

