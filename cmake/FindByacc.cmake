
find_host_program(BYACC_EXECUTABLE byacc PATHS /usr/bin)

if(BYACC_EXECUTABLE)
    execute_process(COMMAND ${BYACC_EXECUTABLE} -V
                    OUTPUT_VARIABLE ${BYACC_EXECUTABLE}_out
                    RESULT_VARIABLE ${BYACC_EXECUTABLE}_error
                    ERROR_VARIABLE ${BYACC_EXECUTABLE}_suppress)
#  if(BYACC_VERSION_STR MATCHES "byacc - ([0-9\\.]+[0-9])")
#    set(BYACC_VERSION "${CMAKE_MATCH_2}")
#  else()
#    set(BYACC_VERSION "unknown")
#  endif()
endif()

if (NOT byacc_error)
    set(BYACC_FOUND 1
        CACHE INTERNAL "byacc version ${byacc_version} found")
endif ()


include(FindPackageHandleStandardArgs)

