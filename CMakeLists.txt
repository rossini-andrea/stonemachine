cmake_minimum_required(VERSION 3.16)
project(stonemachine)

add_executable(stonemachine src/main.cpp)

find_package(Boost)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_executable(carver src/carver.cpp)
  set_property(TARGET carver PROPERTY CXX_STANDARD 17)
endif()
