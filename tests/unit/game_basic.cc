#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "include/xq/game.h"

namespace az::game::xq {
namespace {

TEST(GameBasic, FR_INIT_RED_BACK_GetBoardReturnsExpectedRedBackRank) {
  const XqGame game;
  const XqB& board = game.GetBoard();
  // Spot-check Red back rank, taken straight from the rules doc.
  EXPECT_EQ(board[0 * kBoardCols + 4], 1);   // Red General
  EXPECT_EQ(board[0 * kBoardCols + 0], 5);   // Red Chariot left
  EXPECT_EQ(board[0 * kBoardCols + 8], 5);   // Red Chariot right
}

TEST(GameBasic, GetBoardIsConsistentAcrossCalls) {
  const XqGame game;
  const XqB& a = game.GetBoard();
  const XqB& b = game.GetBoard();
  EXPECT_EQ(&a, &b)
      << "GetBoard() must return a stable reference to the live state.";
}

TEST(GameBasic, FR_INIT_ROUND_ZERO_DefaultGame) {
  const XqGame game;
  EXPECT_EQ(game.CurrentRound(), 0u);
}

TEST(GameBasic, FR_PLAYER_RED_FIRST_DefaultGame) {
  const XqGame game;
  EXPECT_EQ(game.CurrentPlayer(), kRed);
}

TEST(GameBasic, FR_PLAYER_EXPLICIT_BlackStartsViaConstructor) {
  const XqGame game(kBlack);
  EXPECT_EQ(game.CurrentPlayer(), kBlack);
}

TEST(GameBasic, FR_API_LAST_EMPTY_LastPlayerNulloptOnFreshGame) {
  const XqGame game;
  EXPECT_FALSE(game.LastPlayer().has_value());
}

TEST(GameBasic, FR_API_LAST_EMPTY_LastActionNulloptOnFreshGame) {
  const XqGame game;
  EXPECT_FALSE(game.LastAction().has_value());
}

TEST(GameBasic, FR_API_LAST_AFTER_APPLY_LastPlayerEqualsPriorPlayer) {
  XqGame game;
  const std::vector<XqA> actions = game.ValidActions();
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  const XqP prior_player = game.CurrentPlayer();
  game.ApplyActionInPlace(actions.front());
  ASSERT_TRUE(game.LastPlayer().has_value());
  EXPECT_EQ(*game.LastPlayer(), prior_player);
}

TEST(GameBasic, FR_API_LAST_AFTER_APPLY_LastActionEqualsAppliedAction) {
  XqGame game;
  const std::vector<XqA> actions = game.ValidActions();
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  const XqA expected = actions.front();
  game.ApplyActionInPlace(expected);
  ASSERT_TRUE(game.LastAction().has_value());
  EXPECT_EQ(game.LastAction()->from, expected.from);
  EXPECT_EQ(game.LastAction()->to, expected.to);
}

TEST(GameBasic, FR_TURN_INCREMENT_RoundIncreasesAfterApply) {
  XqGame game;
  const std::vector<XqA> actions = game.ValidActions();
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  game.ApplyActionInPlace(actions.front());
  EXPECT_EQ(game.CurrentRound(), 1u);
}

TEST(GameBasic, FR_TURN_ALTERNATE_PlayerFlipsAfterApply) {
  XqGame game;
  const std::vector<XqA> actions = game.ValidActions();
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  const XqP before = game.CurrentPlayer();
  game.ApplyActionInPlace(actions.front());
  EXPECT_NE(game.CurrentPlayer(), before);
}

}  // namespace
}  // namespace az::game::xq
