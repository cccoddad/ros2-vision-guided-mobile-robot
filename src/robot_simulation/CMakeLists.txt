cmake_minimum_required(VERSION 3.8)
project(robot_simulation)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY config launch worlds
  DESTINATION share/${PROJECT_NAME}
)

install(
  PROGRAMS robot_simulation/sim_tag_pose.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
