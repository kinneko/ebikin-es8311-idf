# original implementation by ebikin
# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components/ebikin_es8311")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ebikin_es8311_example)
