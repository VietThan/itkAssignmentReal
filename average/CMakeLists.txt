cmake_minimum_required(VERSION 2.8)

project (average)

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

add_executable (average average.cxx)

target_link_libraries (average ${ITK_LIBRARIES})

