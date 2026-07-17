cmake_minimum_required(VERSION 3.22)

list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../components)
set(SDKCONFIG_DEFAULTS ${CMAKE_CURRENT_LIST_DIR}/../../components/cozmars/sdkconfig.defaults.recommend)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(display_eyes)
