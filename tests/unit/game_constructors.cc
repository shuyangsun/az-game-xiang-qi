#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "include/xq/game.h"
#include "tests/unit/valid_actions.h"

namespace az::game::xq {
namespace {

// Piece codes used in expected starting-position tests.
constexpr int8_t kRedGen = 1;
constexpr int8_t kRedAdv = 2;
constexpr int8_t kRedEle = 3;
constexpr int8_t kRedHor = 4;
constexpr int8_t kRedCha = 5;
constexpr int8_t kRedCan = 6;
constexpr int8_t kRedSol = 7;
constexpr int8_t kBlkGen = -1;
constexpr int8_t kBlkAdv = -2;
constexpr int8_t kBlkEle = -3;
constexpr int8_t kBlkHor = -4;
constexpr int8_t kBlkCha = -5;
constexpr int8_t kBlkCan = -6;
constexpr int8_t kBlkSol = -7;

constexpr size_t Cell(uint8_t row, uint8_t col) noexcept {
  return static_cast<size_t>(row) * kBoardCols + col;
}

XqB ExpectedStartBoard() noexcept {
  XqB board{};
  // Row 0 — Red back rank
  board[Cell(0, 0)] = kRedCha;
  board[Cell(0, 1)] = kRedHor;
  board[Cell(0, 2)] = kRedEle;
  board[Cell(0, 3)] = kRedAdv;
  board[Cell(0, 4)] = kRedGen;
  board[Cell(0, 5)] = kRedAdv;
  board[Cell(0, 6)] = kRedEle;
  board[Cell(0, 7)] = kRedHor;
  board[Cell(0, 8)] = kRedCha;
  // Row 2 — Red cannons
  board[Cell(2, 1)] = kRedCan;
  board[Cell(2, 7)] = kRedCan;
  // Row 3 — Red soldiers
  board[Cell(3, 0)] = kRedSol;
  board[Cell(3, 2)] = kRedSol;
  board[Cell(3, 4)] = kRedSol;
  board[Cell(3, 6)] = kRedSol;
  board[Cell(3, 8)] = kRedSol;
  // Row 6 — Black soldiers
  board[Cell(6, 0)] = kBlkSol;
  board[Cell(6, 2)] = kBlkSol;
  board[Cell(6, 4)] = kBlkSol;
  board[Cell(6, 6)] = kBlkSol;
  board[Cell(6, 8)] = kBlkSol;
  // Row 7 — Black cannons
  board[Cell(7, 1)] = kBlkCan;
  board[Cell(7, 7)] = kBlkCan;
  // Row 9 — Black back rank
  board[Cell(9, 0)] = kBlkCha;
  board[Cell(9, 1)] = kBlkHor;
  board[Cell(9, 2)] = kBlkEle;
  board[Cell(9, 3)] = kBlkAdv;
  board[Cell(9, 4)] = kBlkGen;
  board[Cell(9, 5)] = kBlkAdv;
  board[Cell(9, 6)] = kBlkEle;
  board[Cell(9, 7)] = kBlkHor;
  board[Cell(9, 8)] = kBlkCha;
  return board;
}

TEST(GameConstructors, FR_PLAYER_RED_FIRST_DefaultStartsWithRed) {
  const XqGame game;
  EXPECT_EQ(game.CurrentPlayer(), kRed);
}

TEST(GameConstructors, FR_PLAYER_EXPLICIT_BlackStarts) {
  const XqGame game(kBlack);
  EXPECT_EQ(game.CurrentPlayer(), kBlack);
}

TEST(GameConstructors, FR_PLAYER_EXPLICIT_RedStarts) {
  const XqGame game(kRed);
  EXPECT_EQ(game.CurrentPlayer(), kRed);
}

TEST(GameConstructors, FR_INIT_RED_BACK_DefaultBoard) {
  const XqGame game;
  const XqB& board = game.GetBoard();
  EXPECT_EQ(board[Cell(0, 0)], kRedCha);
  EXPECT_EQ(board[Cell(0, 1)], kRedHor);
  EXPECT_EQ(board[Cell(0, 2)], kRedEle);
  EXPECT_EQ(board[Cell(0, 3)], kRedAdv);
  EXPECT_EQ(board[Cell(0, 4)], kRedGen);
  EXPECT_EQ(board[Cell(0, 5)], kRedAdv);
  EXPECT_EQ(board[Cell(0, 6)], kRedEle);
  EXPECT_EQ(board[Cell(0, 7)], kRedHor);
  EXPECT_EQ(board[Cell(0, 8)], kRedCha);
}

TEST(GameConstructors, FR_INIT_BLACK_BACK_DefaultBoard) {
  const XqGame game;
  const XqB& board = game.GetBoard();
  EXPECT_EQ(board[Cell(9, 0)], kBlkCha);
  EXPECT_EQ(board[Cell(9, 1)], kBlkHor);
  EXPECT_EQ(board[Cell(9, 2)], kBlkEle);
  EXPECT_EQ(board[Cell(9, 3)], kBlkAdv);
  EXPECT_EQ(board[Cell(9, 4)], kBlkGen);
  EXPECT_EQ(board[Cell(9, 5)], kBlkAdv);
  EXPECT_EQ(board[Cell(9, 6)], kBlkEle);
  EXPECT_EQ(board[Cell(9, 7)], kBlkHor);
  EXPECT_EQ(board[Cell(9, 8)], kBlkCha);
}

TEST(GameConstructors, FR_INIT_CANNONS_DefaultBoard) {
  const XqGame game;
  const XqB& board = game.GetBoard();
  EXPECT_EQ(board[Cell(2, 1)], kRedCan);
  EXPECT_EQ(board[Cell(2, 7)], kRedCan);
  EXPECT_EQ(board[Cell(7, 1)], kBlkCan);
  EXPECT_EQ(board[Cell(7, 7)], kBlkCan);
  // No other cannons anywhere on row 2 / row 7.
  for (uint8_t c = 0; c < kBoardCols; ++c) {
    if (c == 1 || c == 7) continue;
    EXPECT_EQ(board[Cell(2, c)], 0);
    EXPECT_EQ(board[Cell(7, c)], 0);
  }
}

TEST(GameConstructors, FR_INIT_SOLDIERS_DefaultBoard) {
  const XqGame game;
  const XqB& board = game.GetBoard();
  for (uint8_t c = 0; c < kBoardCols; ++c) {
    const bool soldier_col = (c % 2 == 0);
    EXPECT_EQ(board[Cell(3, c)], soldier_col ? kRedSol : 0);
    EXPECT_EQ(board[Cell(6, c)], soldier_col ? kBlkSol : 0);
  }
}

TEST(GameConstructors, FR_INIT_EMPTY_CELLS_DefaultBoard) {
  const XqGame game;
  const XqB expected = ExpectedStartBoard();
  for (size_t i = 0; i < expected.size(); ++i) {
    if (expected[i] != 0) continue;
    EXPECT_EQ(game.GetBoard()[i], 0)
        << "Expected empty cell at index " << i << " (row " << (i / kBoardCols)
        << " col " << (i % kBoardCols) << ").";
  }
}

TEST(GameConstructors, FR_INIT_ROUND_ZERO_Default) {
  const XqGame game;
  EXPECT_EQ(game.CurrentRound(), 0u);
}

TEST(GameConstructors, FR_INIT_ROUND_ZERO_Explicit) {
  const XqGame game(kBlack);
  EXPECT_EQ(game.CurrentRound(), 0u);
}

TEST(GameConstructors, FR_INIT_NO_LAST_Default) {
  const XqGame game;
  EXPECT_FALSE(game.LastPlayer().has_value());
  EXPECT_FALSE(game.LastAction().has_value());
}

TEST(GameConstructors, FR_INIT_NO_LAST_Explicit) {
  const XqGame game(kBlack);
  EXPECT_FALSE(game.LastPlayer().has_value());
  EXPECT_FALSE(game.LastAction().has_value());
}

TEST(GameConstructors, FR_MCTS_COPY_PRESERVES_FreshGame) {
  const XqGame game;
  const XqGame copy =
      game;  // NOLINT(performance-unnecessary-copy-initialization)
  EXPECT_EQ(copy.GetBoard(), game.GetBoard());
  EXPECT_EQ(copy.CurrentRound(), game.CurrentRound());
  EXPECT_EQ(copy.CurrentPlayer(), game.CurrentPlayer());
  EXPECT_EQ(copy.LastPlayer(), game.LastPlayer());
  EXPECT_EQ(copy.LastAction().has_value(), game.LastAction().has_value());
}

TEST(GameConstructors, FR_MCTS_COPY_PRESERVES_AfterApply) {
  XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  game.ApplyActionInPlace(actions.front());
  const XqGame copy = game;
  EXPECT_EQ(copy.GetBoard(), game.GetBoard());
  EXPECT_EQ(copy.CurrentRound(), game.CurrentRound());
  EXPECT_EQ(copy.CurrentPlayer(), game.CurrentPlayer());
  EXPECT_EQ(copy.LastPlayer(), game.LastPlayer());
  ASSERT_TRUE(copy.LastAction().has_value());
  ASSERT_TRUE(game.LastAction().has_value());
  EXPECT_EQ(copy.LastAction()->from, game.LastAction()->from);
  EXPECT_EQ(copy.LastAction()->to, game.LastAction()->to);
}

TEST(GameConstructors, FR_MCTS_MOVE_PRESERVES_FreshGame) {
  XqGame original;
  const XqB expected_board = original.GetBoard();
  const XqP expected_player = original.CurrentPlayer();
  const uint32_t expected_round = original.CurrentRound();
  const XqGame moved = std::move(original);
  EXPECT_EQ(moved.GetBoard(), expected_board);
  EXPECT_EQ(moved.CurrentPlayer(), expected_player);
  EXPECT_EQ(moved.CurrentRound(), expected_round);
  EXPECT_FALSE(moved.LastPlayer().has_value());
  EXPECT_FALSE(moved.LastAction().has_value());
}

TEST(GameConstructors, SnapshotConstructorPropagatesFields) {
  const XqB board = ExpectedStartBoard();
  const XqGame game(board, kBlack, /*current_round=*/4,
                    XqA{Cell(0, 1), Cell(2, 2)});
  EXPECT_EQ(game.GetBoard(), board);
  EXPECT_EQ(game.CurrentPlayer(), kBlack);
  EXPECT_EQ(game.CurrentRound(), 4u);
  ASSERT_TRUE(game.LastAction().has_value());
  EXPECT_EQ(game.LastAction()->from, Cell(0, 1));
  EXPECT_EQ(game.LastAction()->to, Cell(2, 2));
  ASSERT_TRUE(game.LastPlayer().has_value());
  EXPECT_EQ(*game.LastPlayer(), kRed);
}

TEST(GameConstructors, SnapshotConstructorWithoutLastAction) {
  const XqB board = ExpectedStartBoard();
  const XqGame game(board, kRed, /*current_round=*/4, std::nullopt);
  EXPECT_EQ(game.CurrentRound(), 4u);
  EXPECT_FALSE(game.LastAction().has_value());
  EXPECT_FALSE(game.LastPlayer().has_value());
}

}  // namespace
}  // namespace az::game::xq
