cmake_minimum_required(VERSION 3.8)
project(base_driver)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(robot_interfaces REQUIRED)

add_library(command_watchdog INTERFACE)
target_include_directories(command_watchdog INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_library(base_transport INTERFACE)
target_include_directories(base_transport INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_executable(base_driver_node src/base_driver_node.cpp)
target_link_libraries(base_driver_node command_watchdog)
ament_target_dependencies(base_driver_node
  geometry_msgs
  rclcpp
  robot_interfaces
)

install(TARGETS base_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_command_watchdog test/test_command_watchdog.cpp)
  target_link_libraries(test_command_watchdog command_watchdog)

  ament_add_gtest(test_fake_transport test/test_fake_transport.cpp)
  target_link_libraries(test_fake_transport base_transport)
endif()

ament_package()
