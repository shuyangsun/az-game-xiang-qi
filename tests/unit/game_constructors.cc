#include "gtest/gtest.h"
#include "include/xq/game.h"

namespace az::game::xq {
namespace {

using ::az::game::xq::XqB;
using ::az::game::xq::XqGame;
using ::az::game::xq::XqP;

// TODO(TASK-GAME-CONSTRUCTOR-TEST): delete this test if the default
// constructor was deleted.
TEST(GameConstructors, DefaultConstructor) {
  XqGame game;
  EXPECT_EQ(game.GetBoard(), XqB{});
  EXPECT_EQ(game.CurrentRound(), 0u);
  EXPECT_EQ(game.CurrentPlayer(), XqP{});
  EXPECT_EQ(game.LastPlayer(), std::nullopt);
  EXPECT_FALSE(game.LastAction().has_value());
}

TEST(GameConstructors, ExplicitStartingPlayer) {
  GTEST_SKIP() << "TODO(TASK-GAME-CONSTRUCTOR-TEST): verify the "
                  "explicit-starting-player constructor.";
}

TEST(GameConstructors, CopyConstructorPreservesState) {
  GTEST_SKIP() << "TODO(TASK-GAME-CONSTRUCTOR-TEST): verify the copy "
                  "constructor preserves state.";
}

TEST(GameConstructors, MoveConstructorPreservesState) {
  GTEST_SKIP() << "TODO(TASK-GAME-CONSTRUCTOR-TEST): verify the move "
                  "constructor preserves state.";
}

}  // namespace
}  // namespace az::game::xq
