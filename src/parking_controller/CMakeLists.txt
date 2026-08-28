cmake_minimum_required(VERSION 3.8)
project(parking_controller)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

ament_python_install_package(${PROJECT_NAME})

install(
  PROGRAMS parking_controller/parking_controller.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME parking_controller
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
