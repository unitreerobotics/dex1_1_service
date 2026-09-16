cmake_minimum_required(VERSION 3.10)
project(Dex1_1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  message("-- BUILD_TYPE: Debug")
  add_compile_options(-g -O0 -fPIC)
else()
  message("-- BUILD_TYPE: Release")
  add_compile_options(-O3 -DNDEBUG -fPIC)
endif()

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
)

link_directories(
  ${CMAKE_SOURCE_DIR}/lib
)
link_libraries(
  rt pthread
  libboost_program_options.a fmt
)

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64") 
    link_libraries( libUnitreeMotorSDK_Arm64.so)
else()
    link_libraries( libUnitreeMotorSDK_Linux64.so)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_executable(dex1_1_internal_calibration main.cpp)