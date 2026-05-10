include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(googletest)

# Suppress gtest-printers.h warning (char8_t -> char32_t)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wcharacter-conversion _HAS_WCHARACTER_CONVERSION)
if(_HAS_WCHARACTER_CONVERSION)
  foreach(_gt_target gtest gtest_main gmock gmock_main)
    if(TARGET ${_gt_target})
      target_compile_options(${_gt_target} PRIVATE -Wno-error=character-conversion)
    endif()
  endforeach()
endif()

add_subdirectory(unit)
