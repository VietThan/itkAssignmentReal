cmake_minimum_required(VERSION 2.8)

project (deformed)

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

add_executable (deformed deformed.cxx)

target_link_libraries (deformed ${ITK_LIBRARIES})

