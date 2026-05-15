if(EMSCRIPTEN)
  add_executable(xq_wasm bindings.cc)
  
  target_include_directories(xq_wasm PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(xq_wasm PRIVATE game_xq serializer_xq inference_xq)

  set_target_properties(xq_wasm PROPERTIES
    OUTPUT_NAME "xq"
    SUFFIX ".js"
  )

  target_link_options(xq_wasm PRIVATE
    -lembind
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createXqModule"
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap"
    "-sALLOW_MEMORY_GROWTH=1"
  )

  # Copy the generated .js and .wasm to the GUI public folder
  add_custom_command(TARGET xq_wasm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/gui/public/wasm
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/xq.js
      ${PROJECT_SOURCE_DIR}/gui/public/wasm/xq.js
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/xq.wasm
      ${PROJECT_SOURCE_DIR}/gui/public/wasm/xq.wasm
  )
endif()
