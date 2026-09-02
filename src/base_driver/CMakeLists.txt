cmake_minimum_required(VERSION 3.8)
project(base_driver LANGUAGES C CXX)

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

add_library(driver_configuration INTERFACE)
target_include_directories(driver_configuration INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_library(base_transport INTERFACE)
target_include_directories(base_transport INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_library(robot_protocol STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol/src/robot_protocol.c
)
set_target_properties(robot_protocol PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
target_include_directories(robot_protocol PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol/include
)

add_library(base_status_receiver src/base_status_receiver.cpp)
target_link_libraries(base_status_receiver PUBLIC base_transport robot_protocol)
ament_target_dependencies(base_status_receiver PUBLIC robot_interfaces)

add_library(driver_health INTERFACE)
target_include_directories(driver_health INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(driver_health INTERFACE base_status_receiver)

add_library(command_frame_encoder src/command_frame_encoder.cpp)
target_link_libraries(command_frame_encoder PUBLIC base_transport robot_protocol)

add_executable(base_driver_node src/base_driver_node.cpp)
target_link_libraries(base_driver_node command_watchdog driver_configuration)
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

  ament_add_gtest(test_base_status_receiver test/test_base_status_receiver.cpp)
  target_link_libraries(test_base_status_receiver base_status_receiver)

  ament_add_gtest(test_driver_health test/test_driver_health.cpp)
  target_link_libraries(test_driver_health driver_health)

  ament_add_gtest(test_command_frame_encoder test/test_command_frame_encoder.cpp)
  target_link_libraries(test_command_frame_encoder command_frame_encoder)

  ament_add_gtest(test_driver_configuration test/test_driver_configuration.cpp)
  target_link_libraries(test_driver_configuration driver_configuration)
endif()

ament_package()
