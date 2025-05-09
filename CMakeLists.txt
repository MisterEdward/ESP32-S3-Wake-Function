cmake_minimum_required(VERSION 3.5)

# No need for EXTRA_COMPONENT_DIRS since components directory is automatically searched
# Libraries have been moved from managed_components to components

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-remote-wakeup)
