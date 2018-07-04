cmake_minimum_required(VERSION 3.0)

set(TFTPS_PROGRAM tftps)

project(${TFTPS_PROGRAM} C)

set(CMAKE_C_STANDARD 11)
# The version number.
set(TFTPC_VERSION_MAJOR 1)
set(TFTPC_VERSION_MINOR 0)
set(TFTPC_VERSION_PATCH 0)
set(TFTPC_VERSION ${TFTPC_VERSION_MAJOR}.${TFTPC_VERSION_MINOR}.${TFTPC_VERSION_PATCH})


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

set(PROJECT_H_FILES tftp_lib.h tftp_types.h tftp_resources.h tftps_main.h)
set(PROJECT_C_FILES tftp_lib.c tftp_resources.c tftps_main.c)

configure_file(
        "${CMAKE_SOURCE_DIR}/${TFTPS_PROGRAM}_config.h.in"
        "${CMAKE_BINARY_DIR}/${TFTPS_PROGRAM}_config.h")

install(FILES ${PROJECT_H_FILES}
        DESTINATION "${TARGET_INCLUDE_DIR}")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif ()


set(CMAKE_C_COMPILER "gcc")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${TARGET_INCLUDE_DIR})

add_executable(${TFTPS_PROGRAM} ${PROJECT_C_FILES})

target_compile_options(${TFTPS_PROGRAM} PUBLIC -Wall -Wextra -pedantic)

