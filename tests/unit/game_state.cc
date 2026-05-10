#include "gtest/gtest.h"

namespace az::game::xq {
namespace {

TEST(GameState, CanonicalBoardReturnsExpectedView) {
  GTEST_SKIP() << "TODO(TASK-GAME-STATE-TEST): verify CanonicalBoard() for the "
                  "current player.";
}

TEST(GameState, IsOverReturnsFalseAtStart) {
  GTEST_SKIP()
      << "TODO(TASK-GAME-STATE-TEST): verify the initial game is not over.";
}

TEST(GameState, GetScoreReturnsExpectedTerminalScores) {
  GTEST_SKIP()
      << "TODO(TASK-GAME-STATE-TEST): verify terminal scores for each player.";
}

}  // namespace
}  // namespace az::game::xq
