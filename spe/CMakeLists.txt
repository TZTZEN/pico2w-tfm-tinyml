#-------------------------------------------------------------------------------
# Copyright (c) 2023-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/utils)
include(utils)
include(tinyml_flag_parse)

collect_build_cmd_args(TFM_CMDLINE_CONFIGS)

project("TF-M SPE for tinyml" NONE)

if (NOT DEFINED CONFIG_TFM_SOURCE_PATH OR NOT EXISTS ${CONFIG_TFM_SOURCE_PATH})
    message(FATAL_ERROR "CONFIG_TFM_SOURCE_PATH = ${CONFIG_TFM_SOURCE_PATH} is not defined or incorrect. Please provide full path to TF-M sources.")
endif()

# Parse tinyml flags (ensures all test flags are disabled)
parse_tinyml_flag(TFM_CMDLINE_CONFIGS)

# tinyml application does not need regression tests - force all test flags to OFF
set(TFM_NS_REG_TEST OFF CACHE BOOL "Non-secure regression tests are disabled for tinyml" FORCE)
set(TFM_S_REG_TEST OFF CACHE BOOL "Secure regression tests are disabled for tinyml" FORCE)
set(TEST_BL2 OFF CACHE BOOL "BL2 tests are disabled for tinyml" FORCE)
set(TEST_BL1_1 OFF CACHE BOOL "BL1_1 tests are disabled for tinyml" FORCE)
set(TEST_BL1_2 OFF CACHE BOOL "BL1_2 tests are disabled for tinyml" FORCE)

# tfm_s_tinyml is IMPORTED to inform CMake that it has no source files.
add_executable(tfm_s_tinyml IMPORTED)

# Use TINYML-specific configuration file
set(CONFIG_TINYML_CONFIG_FILE ${CMAKE_CURRENT_LIST_DIR}/config/config_tinyml.cmake)

include(ExternalProject)

ExternalProject_Add(TF-M
  SOURCE_DIR        ${CONFIG_TFM_SOURCE_PATH}
  BINARY_DIR        build-spe
  INSTALL_DIR       api_ns
  CMAKE_ARGS        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  CMAKE_ARGS        -DPROJECT_CONFIG_HEADER_FILE=${CMAKE_CURRENT_LIST_DIR}/config/config_tinyml.h
  CMAKE_ARGS        -DCONFIG_TINYML_CONFIG_FILE=${CONFIG_TINYML_CONFIG_FILE}
  CMAKE_CACHE_DEFAULT_ARGS ${TFM_CMDLINE_CONFIGS}
  PREFIX             "temp"
  BUILD_ALWAYS       ON
)

add_dependencies(tfm_s_tinyml TF-M)

# Skip "up-to-date" prints to avoid flooding the build output. Just print "installing"
set(CMAKE_INSTALL_MESSAGE LAZY)

install(DIRECTORY ${CMAKE_BINARY_DIR}/build-spe/bin DESTINATION ${CMAKE_BINARY_DIR})

install(DIRECTORY   ${CONFIG_TFM_SOURCE_PATH}/secure_fw/partitions/initial_attestation
        DESTINATION ${CMAKE_BINARY_DIR}/api_ns
        FILES_MATCHING PATTERN "*.h")

install(FILES       ${CONFIG_TFM_SOURCE_PATH}/secure_fw/spm/include/boot/tfm_boot_status.h
        DESTINATION ${CMAKE_BINARY_DIR}/api_ns/initial_attestation)

# Install echo service interface files
install(FILES       ${CMAKE_CURRENT_LIST_DIR}/../interface/include/tfm_echo_defs.h
        DESTINATION ${CMAKE_BINARY_DIR}/api_ns/interface/include)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/../interface/src/tfm_echo_api.c
        DESTINATION ${CMAKE_BINARY_DIR}/api_ns/interface/src)

# Install TinyMaix inference service interface files
install(FILES       ${CMAKE_CURRENT_LIST_DIR}/../interface/include/tfm_tinymaix_inference_defs.h
        DESTINATION ${CMAKE_BINARY_DIR}/api_ns/interface/include)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/../interface/src/tfm_tinymaix_inference_api.c
        DESTINATION ${CMAKE_BINARY_DIR}/api_ns/interface/src)
