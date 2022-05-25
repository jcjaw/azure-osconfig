# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

function(generate_module target name)
    message("Generating ${name}.so")

    set(MODULE_NAME ${name})
    string(TOLOWER ${MODULE_NAME} MODULE_NAME_LOWER)
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

    configure_file(
        ${MODULES_TEMPLATE_DIR}/Log.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/inc/${MODULE_NAME}Log.h
        @ONLY)

    configure_file(
        ${MODULES_TEMPLATE_DIR}/Log.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/lib/${MODULE_NAME}Log.cpp
        @ONLY)

    configure_file(
        ${MODULES_TEMPLATE_DIR}/Module.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/so/${MODULE_NAME}Module.cpp
        @ONLY)

    set(MODULE_LOGGING ${MODULE_NAME_LOWER}_logging)

    add_library(${MODULE_LOGGING} STATIC ${CMAKE_CURRENT_BINARY_DIR}/lib/Log.cpp)
    target_link_libraries(${MODULE_LOGGING} logging)
    target_include_directories(${MODULE_LOGGING} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/inc)

    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/inc)

    set(MODULE_SO ${MODULE_NAME_LOWER})

    add_library(${MODULE_SO} SHARED ${CMAKE_CURRENT_BINARY_DIR}/so/${MODULE_NAME}Module.cpp)
    add_dependencies(${MODULE_SO} ${target})

    target_link_libraries(${MODULE_SO}
        PRIVATE
            commonutils
            ${MODULE_LOGGING}
        PUBLIC
            ${target})

    target_include_directories(${MODULE_SO} PUBLIC ${MODULES_INC_DIR})

    set_target_properties(${MODULE_SO}
        PROPERTIES
            PREFIX ""
            POSITION_INDEPENDENT_CODE ON)

    install(TARGETS ${MODULE_SO} DESTINATION ${MODULES_INSTALL_DIR})
endfunction()