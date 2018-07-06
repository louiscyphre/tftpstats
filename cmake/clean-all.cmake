set(cmake_generated ${CMAKE_SOURCE_DIR}/CMakeCache.txt
                    ${CMAKE_SOURCE_DIR}/cmake_install.cmake
                    ${CMAKE_SOURCE_DIR}/Makefile
                    ${CMAKE_SOURCE_DIR}/CMakeFiles
                    ${CMAKE_SOURCE_DIR}/cmake-build-debug
                    ${CMAKE_SOURCE_DIR}/pcap-prefix
                    ${CMAKE_SOURCE_DIR}/tftpc-prefix
                    ${CMAKE_SOURCE_DIR}/tftps-prefix
                    ${CMAKE_SOURCE_DIR}/tmp
                    ${CMAKE_SOURCE_DIR}/tftpstats
                    ${CMAKE_BINARY_DIR}/bin
)

foreach(file ${cmake_generated})

  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()

endforeach(file)

