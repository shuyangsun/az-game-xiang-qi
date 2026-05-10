include(GoogleTest)

function(add_game_unit_test target_name source_name)
  add_executable(${target_name} ${source_name})
  target_link_libraries(
    ${target_name}
    PRIVATE GTest::gtest_main
            game_xq
            serializer_xq
            deserializer_xq
            augmentation_xq
            inference_xq
            train_xq)
  gtest_discover_tests(${target_name})
endfunction()

# TODO(TASK-UNITTESTS-PASS): make sure all these tests pass, fix implementation if they don't.
add_game_unit_test(game_constructors_test game_constructors.cc)
add_game_unit_test(game_basic_test game_basic.cc)
add_game_unit_test(game_state_test game_state.cc)
add_game_unit_test(game_string_conv_test game_string_conv.cc)
add_game_unit_test(game_action_test game_action.cc)
add_game_unit_test(serializer_test serializer.cc)
add_game_unit_test(deserializer_test deserializer.cc)
add_game_unit_test(augmentation_test augmentation.cc)
add_game_unit_test(inference_test inference.cc)
add_game_unit_test(train_test train.cc)
