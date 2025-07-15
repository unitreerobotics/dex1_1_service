cmake_minimum_required(VERSION 3.10)
project(Dex1_1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  message("-- BUILD_TYPE: Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -O0 -fPIC")
else()
  message("-- BUILD_TYPE: Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -DNDEBUG -fPIC")
endif()

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(yaml-cpp REQUIRED)

include_directories(
  /usr/local/include/ddscxx
  /usr/local/include/iceoryx/v2.0.2
  ${CMAKE_SOURCE_DIR}/include
)

link_directories(
  ${CMAKE_SOURCE_DIR}/lib
)
link_libraries(
  unitree_sdk2 ddsc ddscxx rt pthread
  libboost_program_options.a yaml-cpp fmt
)

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64") 
    link_libraries( libUnitreeMotorSDK_Arm64.so)
else()
    link_libraries( libUnitreeMotorSDK_Linux64.so)
endif()

add_executable(dex1_1_gripper_server main.cpp)
add_executable(test_dex1_1_gripper_server test/test_gripper.cpp)
