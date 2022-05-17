# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(osconfig-functional-tests)

add_executable(oft MimParser.cpp)

target_link_libraries(oft
    ${CMAKE_DL_LIBS}
    pthread
    logging
    commonutils
    parsonlib)