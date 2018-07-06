# Enable build defense flags.
# Performance may be affected.
# More information:
# - https://www.owasp.org/index.php/C-Based_Toolchain_Hardening
# - https://wiki.debian.org/Hardening
# - https://wiki.gentoo.org/wiki/Hardened/Toolchain
# - https://docs.microsoft.com/en-us/cpp/build/reference/sdl-enable-additional-security-checks


set(TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON "")

macro(tftpstats_add_defense_compiler_flag option)
  tftpstats_check_flag_support(C "${option}" _varname "${ARGN}")
  if(${_varname})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${option}")
  endif()
endmacro()

macro(tftpstats_add_defense_compiler_flag_release option)
  tftpstats_check_flag_support(C "${option}" _varname "${ARGN}")
  if(${_varname})
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${option}")
  endif()
endmacro()

# Define flags

if(MSVC)
  tftpstats_add_defense_compiler_flag("/GS")
  tftpstats_add_defense_compiler_flag("/sdl")
  tftpstats_add_defense_compiler_flag("/guard:cf")
  tftpstats_add_defense_compiler_flag("/w34018 /w34146 /w34244 /w34267 /w34302 /w34308 /w34509 /w34532 /w34533 /w34700 /w34789 /w34995 /w34996")
  set(TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON "${TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON} /guard:cf /dynamicbase" )
  if(NOT X86_64)
    set(TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON "${TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON} /safeseh")
  endif()
elseif(TFTPSTATS_GCC)
  if(CMAKE_C_COMPILER_VERSION VERSION_LESS "4.9")
    tftpstats_add_defense_compiler_flag("-fstack-protector")
  else()
    tftpstats_add_defense_compiler_flag("-fstack-protector-strong")
  endif()

  include(CheckCCompilerFlag)
  check_c_compiler_flag(-Wformat HAS_WFORMAT)
  if (HAS_WFORMAT)
     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat")
  endif()
  check_c_compiler_flag(-Wformat-security HAS_WFORMAT_SECURITY)
  if (HAS_WFORMAT_SECURITY)
     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat-security")
  endif()

  if(ANDROID)
    tftpstats_add_defense_compiler_flag_release("-D_FORTIFY_SOURCE=2")
    if(NOT CMAKE_C_FLAGS_RELEASE MATCHES "-D_FORTIFY_SOURCE=2") # TODO Check this
      tftpstats_add_defense_compiler_flag_release("-D_FORTIFY_SOURCE=1")
    endif()
  else()
    tftpstats_add_defense_compiler_flag_release("-D_FORTIFY_SOURCE=2")
  endif()

  set(TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON "${TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON} -z noexecstack -z relro -z now" )
else()
  # not supported
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(TFTPSTATS_GCC OR TFTPSTATS_CLANG)
    if(NOT CMAKE_C_FLAGS MATCHES "-fPIC")
      tftpstats_add_defense_compiler_flag("-fPIC")
    endif()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")
endif()

set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON}" )
set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON}" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TFTPSTATS_LINKER_DEFENSES_FLAGS_COMMON}" )

if(TFTPSTATS_GCC OR TFTPSTATS_CLANG)
  foreach(flags
          CMAKE_C_FLAGS CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_DEBUG)
    string(REPLACE "-O3" "-O2" ${flags} "${${flags}}")
  endforeach()
endif()

