add_subdirectory(xq)

add_executable(xq main.cc)
set_target_properties(xq PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
target_include_directories(xq PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(xq PRIVATE game_xq serializer_xq deserializer_xq inference_xq train_xq)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(APPLE)
    target_link_options(xq PRIVATE -Wl,-dead_strip -Wl,-x)
  elseif(UNIX)
    target_link_options(xq PRIVATE -Wl,--gc-sections -Wl,--discard-all)
  endif()
endif()
