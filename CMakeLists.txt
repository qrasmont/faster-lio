cmake_minimum_required(VERSION 3.14.4)
project(faster_lio)

set(DEFAULT_BUILD_TYPE "Release")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fexceptions -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS}")

add_definitions(-DROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/\")

find_package(Glog REQUIRED)
include_directories(${Glog_INCLUDE_DIRS})

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(livox_ros_driver2 REQUIRED)
find_package(rosbag2_cpp REQUIRED)

set(dependencies
    rclcpp
    geometry_msgs
    nav_msgs
    sensor_msgs
    std_msgs
    pcl_ros
    pcl_conversions
    livox_ros_driver2
    rosbag2_cpp
)

rosidl_generate_interfaces(${PROJECT_NAME}
    "msg/Pose6D.msg"
    DEPENDENCIES geometry_msgs
)

ament_export_dependencies(rosidl_default_runtime)

find_package(Eigen3 REQUIRED)
find_package(PCL 1.8 REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(
    ${EIGEN3_INCLUDE_DIR}
    ${PCL_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${yaml-cpp_INCLUDE_DIRS}
    include
)
# options
option(WITH_IVOX_NODE_TYPE_PHC "Use PHC instead of default ivox node" OFF)

if (WITH_IVOX_NODE_TYPE_PHC)
    message("USING_IVOX_NODE_TYPE_PHC")
    add_definitions(-DIVOX_NODE_TYPE_PHC)
else ()
    message("USING_IVOX_NODE_TYPE_DEFAULT")
endif()

add_library(laser_mapping SHARED
    src/laser_mapping.cc
    src/pointcloud_preprocess.cc
    src/options.cc
    src/utils.cc
)

target_link_libraries(laser_mapping
    ${PCL_LIBRARIES}
    ${PYTHON_LIBRARIES}
    tbb
    glog
    yaml-cpp
)

target_include_directories(laser_mapping PRIVATE ${PYTHON_INCLUDE_DIRS})
ament_target_dependencies(laser_mapping ${dependencies})
rosidl_target_interfaces(laser_mapping
    ${PROJECT_NAME} "rosidl_typesupport_cpp")

add_executable(run_mapping_online app/run_mapping_online.cc)
target_link_libraries(run_mapping_online
    laser_mapping
    gflags
)
ament_target_dependencies(run_mapping_online ${dependencies})

add_executable(run_mapping_offline app/run_mapping_offline.cc)
target_link_libraries(run_mapping_offline
        laser_mapping
        gflags
        rosbag2_cpp::rosbag2_cpp
        )
ament_target_dependencies(run_mapping_offline ${dependencies})

install(
    TARGETS laser_mapping run_mapping_online run_mapping_offline
    DESTINATION lib/${PROJECT_NAME}
)

install(
    TARGETS laser_mapping run_mapping_online
    DESTINATION lib/${PROJECT_NAME}
)

install(
    TARGETS laser_mapping
    LIBRARY DESTINATION lib
)

install(
    DIRECTORY config launch rviz
    DESTINATION share/${PROJECT_NAME}
)

ament_package()
