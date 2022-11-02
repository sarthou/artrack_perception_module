cmake_minimum_required(VERSION 3.0.2)
project(artrack_perception_module)

add_compile_options(-std=c++14)

find_package(catkin REQUIRED COMPONENTS
  ar_track_alvar_msgs
  overworld
  pluginlib
  roscpp
)

catkin_package()

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
  src/ArTrackPerceptionModule.cpp
)
target_link_libraries(${PROJECT_NAME} PUBLIC
                      ${catkin_LIBRARIES}
                      ${overworld_LIBRARIES})