cmake_minimum_required(VERSION 3.8)
project(parking_controller)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(robot_interfaces REQUIRED)

add_library(parking_control_math src/control_math.cpp)
target_include_directories(parking_control_math PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_executable(parking_controller src/parking_controller_node.cpp)
target_link_libraries(parking_controller parking_control_math)
ament_target_dependencies(parking_controller
  geometry_msgs
  rclcpp
  rclcpp_action
  robot_interfaces
)

install(TARGETS parking_controller
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_control_math test/test_control_math.cpp)
  target_link_libraries(test_control_math parking_control_math)
endif()

ament_package()
