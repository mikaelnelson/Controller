cmake_minimum_required(VERSION 3.16.0)
message("IDF PATH $ENV{IDF_PATH}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

list( APPEND EXTRA_COMPONENT_DIRS components/pubsub-c )


project(Controller)
