
find_host_program(FLEX_EXECUTABLE flex PATHS /usr/bin)

if(FLEX_EXECUTABLE)
    execute_process(COMMAND ${FLEX_EXECUTABLE} -V
                    OUTPUT_VARIABLE ${FLEX_EXECUTABLE}_out
                    RESULT_VARIABLE ${FLEX_EXECUTABLE}_error
                    ERROR_VARIABLE ${FLEX_EXECUTABLE}_suppress)
#  if(FLEX_VERSION_STR MATCHES "flex - ([0-9\\.]+[0-9])")
#    set(FLEX_VERSION "${CMAKE_MATCH_2}")
#  else()
#    set(FLEX_VERSION "unknown")
#  endif()
endif()

if (NOT flex_error)
    set(FLEX_FOUND 1
        CACHE INTERNAL "flex version ${flex_version} found")
endif ()


include(FindPackageHandleStandardArgs)
