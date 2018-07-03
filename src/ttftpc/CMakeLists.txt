cmake_minimum_required(VERSION 3.0)

set(TFTPC_PROGRAM ttftpc)

project(${TFTPC_PROGRAM} C)

set(CMAKE_C_STANDARD 11)
# The version number.
set(TTFTPC_VERSION_MAJOR 1)
set(TTFTPC_VERSION_MINOR 0)
set(TTFTPC_VERSION_PATCH 0)
set(TTFTPC_VERSION ${TTFTPC_VERSION_MAJOR}.${TTFTPC_VERSION_MINOR}.${TTFTPC_VERSION_PATCH})


set(CMAKE_COLOR_MAKEFILE ON)

# Disable in-source builds to prevent source tree corruption.
if (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif ()


if (DEPENDENCY_PROJECT AND ROOT_INCLUDE_DIR)
    set(TARGET_INCLUDE_DIR "${ROOT_INCLUDE_DIR}")
elseif (NOT DEPENDENCY_PROJECT AND NOT ROOT_INCLUDE_DIR)
    set(TARGET_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}")
else ()
    message(FATAL_ERROR "
        FATAL: Check ROOT_INCLUDE_DIR and DEPENDENCY_PROJECT defines.
                       They both must be or set or unset.
        ")
endif ()

set(PROJECT_H_FILES ttftp_lib.h ttftp_types.h ttftp_resources.h)
set(PROJECT_C_FILES ttftp_lib.c ttftp_resources.c ttftpc_driver.c)

configure_file(
        "${CMAKE_SOURCE_DIR}/${TFTPC_PROGRAM}_config.h.in"
        "${CMAKE_BINARY_DIR}/${TFTPC_PROGRAM}_config.h")

install(FILES ${PROJECT_H_FILES}
        DESTINATION "${TARGET_INCLUDE_DIR}")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif ()


set(CMAKE_C_COMPILER "gcc")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${TARGET_INCLUDE_DIR})

add_library(${TFTPC_PROGRAM} STATIC ttftp_lib.c ttftp_lib.h ttftp_types.h
        ttftp_resources.c ttftp_resources.h ttftp_types.h ttftp_lib.h
        ttftpc_config.h ttftpc_driver.c)

target_compile_options(ttftpc PUBLIC -Wall -Wextra -pedantic)

