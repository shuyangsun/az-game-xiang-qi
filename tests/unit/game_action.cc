#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "include/xq/game.h"
#include "tests/unit/valid_actions.h"

namespace az::game::xq {
namespace {

// Piece-code shorthand.
constexpr int8_t kRG = 1;
constexpr int8_t kRA = 2;
constexpr int8_t kRE = 3;
constexpr int8_t kRH = 4;
constexpr int8_t kRC = 5;
constexpr int8_t kRP = 6;  // P = paotai (cannon)
constexpr int8_t kRS = 7;
constexpr int8_t kBG = -1;
constexpr int8_t kBA = -2;
constexpr int8_t kBE = -3;
constexpr int8_t kBC = -5;
constexpr int8_t kBS = -7;
[[maybe_unused]] constexpr int8_t kBH = -4;
[[maybe_unused]] constexpr int8_t kBP = -6;

constexpr uint8_t Idx(uint8_t row, uint8_t col) noexcept {
  return static_cast<uint8_t>(row * kBoardCols + col);
}

XqB EmptyBoard() noexcept { return XqB{}; }

// Build a snapshot game with the given piece placements and side
// to move. Convenient for piece-by-piece move-generation tests.
XqGame MakeGame(
    std::initializer_list<std::pair<std::pair<uint8_t, uint8_t>, int8_t>>
        placements,
    XqP to_move) noexcept {
  XqB board = EmptyBoard();
  for (const auto& [rc, code] : placements) {
    board[Idx(rc.first, rc.second)] = code;
  }
  return XqGame(board, to_move, /*current_round=*/0, std::nullopt);
}

// True iff `actions` contains a move with the given (from, to) cells.
bool HasMove(const std::vector<XqA>& actions, uint8_t from, uint8_t to) {
  for (const XqA& a : actions) {
    if (a.from == from && a.to == to) return true;
  }
  return false;
}

// Filter a `ValidActions()` list down to moves originating at one cell.
std::vector<XqA> MovesFrom(const std::vector<XqA>& actions, uint8_t from) {
  std::vector<XqA> out;
  for (const XqA& a : actions) {
    if (a.from == from) out.push_back(a);
  }
  return out;
}

// ----------------------------- PolicyIndex --------------------------------

TEST(GameAction, FR_API_POLICY_BIJECTION_PolicyIndex) {
  const XqGame game;
  std::set<size_t> seen;
  for (uint8_t f = 0; f < kBoardCells; ++f) {
    for (uint8_t t = 0; t < kBoardCells; ++t) {
      const size_t idx = game.PolicyIndex(XqA{f, t});
      EXPECT_LT(idx, XqGame::kPolicySize);
      EXPECT_TRUE(seen.insert(idx).second)
          << "PolicyIndex collision at (" << static_cast<int>(f) << ", "
          << static_cast<int>(t) << ") -> " << idx;
    }
  }
  EXPECT_EQ(seen.size(), XqGame::kPolicySize);
}

TEST(GameAction, FR_API_POLICY_BIJECTION_FromAndToFormula) {
  const XqGame game;
  // Pin the documented mapping: PolicyIndex(a) = from*kBoardCells + to.
  EXPECT_EQ(game.PolicyIndex(XqA{0, 0}), 0u);
  EXPECT_EQ(game.PolicyIndex(XqA{0, 1}), 1u);
  EXPECT_EQ(game.PolicyIndex(XqA{1, 0}), kBoardCells);
  EXPECT_EQ(game.PolicyIndex(XqA{2, 3}), 2u * kBoardCells + 3u);
  EXPECT_EQ(game.PolicyIndex(XqA{kBoardCells - 1, kBoardCells - 1}),
            XqGame::kPolicySize - 1);
}

// --------------------- ValidActions: contract checks ----------------------

TEST(GameAction, FR_API_VALID_DETERMINISTIC_Initial) {
  const XqGame game;
  const std::vector<XqA> a = ValidActions(game);
  const std::vector<XqA> b = ValidActions(game);
  ASSERT_EQ(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i].from, b[i].from);
    EXPECT_EQ(a[i].to, b[i].to);
  }
}

TEST(GameAction, FR_API_VALID_NO_DUPES_Initial) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  std::set<std::pair<uint8_t, uint8_t>> seen;
  for (const XqA& a : actions) {
    EXPECT_TRUE(seen.insert({a.from, a.to}).second)
        << "Duplicate move (" << static_cast<int>(a.from) << ", "
        << static_cast<int>(a.to) << ").";
  }
}

TEST(GameAction, FR_NO_PASS_NoSentinelInValidActions) {
  const XqGame game;
  for (const XqA& a : ValidActions(game)) {
    EXPECT_LT(a.from, kBoardCells);
    EXPECT_LT(a.to, kBoardCells);
    EXPECT_NE(a.from, a.to);  // null move sentinel
  }
}

TEST(GameAction, FR_TERM_EMPTY_VALID_RedFreshGameNotEmpty) {
  const XqGame game;
  EXPECT_FALSE(ValidActions(game).empty());
  EXPECT_FALSE(game.IsOver());
}

// --------------------- General (FR-GEN-*) ---------------------------------

TEST(GameAction, FR_GEN_ORTH_1_GeneralMovesOneOrthogonal) {
  // Red General at palace center (1, 4); only own General on the board
  // plus the opponent General sitting somewhere far so the side-to-move
  // is well-defined and Flying General is irrelevant.
  const XqGame game = MakeGame({{{1, 4}, kRG}, {{9, 0}, kBG}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(1, 4));
  // The General can move to (0,4), (2,4), (1,3), (1,5).
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(0, 4)));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(2, 4)));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(1, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(1, 5)));
  EXPECT_EQ(moves.size(), 4u);
}

TEST(GameAction, FR_GEN_PALACE_GeneralCannotLeavePalace) {
  // Red General at (2, 5) — palace edge; (2, 6) is outside palace.
  const XqGame game = MakeGame({{{2, 5}, kRG}, {{9, 3}, kBG}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(2, 5));
  for (const XqA& a : moves) {
    const uint8_t r = a.to / kBoardCols;
    const uint8_t c = a.to % kBoardCols;
    EXPECT_GE(r, 0u);
    EXPECT_LE(r, 2u);
    EXPECT_GE(c, 3u);
    EXPECT_LE(c, 5u);
  }
}

TEST(GameAction, FR_GEN_FLYING_BlocksOpenFile) {
  // Red General at (0, 4), Black General at (9, 4), no pieces between.
  // Red is to move. Any move that keeps the file open is fine; but a
  // move that doesn't address the open file is not blocked.
  // The face-to-face is *already* blocking — meaning Red's General
  // currently cannot stay still (that's not a move). We test that
  // moves that would still leave the file open (and aren't moving the
  // General off it) are filtered.
  // Setup: Red Chariot at (3, 4) pinned; if Red moves it off the
  // file the Generals would face. Filter must remove that move.
  // Both generals on col 4 with the Red Chariot blocking. Moving the
  // chariot off col 4 must leave the column completely empty between
  // them, which is a Flying Generals violation. The filter must
  // remove every such move.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 4}, kBG}, {{3, 4}, kRC}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(3, 4));
  // ValidActions might still be empty if the position itself is
  // already terminal; but as long as the chariot move list contains
  // any moves, every one must keep col 4 occupied (i.e., the chariot
  // stays on col 4).
  for (const XqA& a : moves) {
    const uint8_t to_col = a.to % kBoardCols;
    EXPECT_EQ(to_col, 4u) << "Move (" << static_cast<int>(a.from) << "->"
                          << static_cast<int>(a.to)
                          << ") would expose the Generals "
                          << "to a flying-general violation.";
  }
}

// --------------------- Advisor (FR-ADV-*) ---------------------------------

TEST(GameAction, FR_ADV_DIAG_1_AdvisorDiagonalMoves) {
  // Red Advisor at palace center (1, 4) — can reach 4 corners.
  const XqGame game =
      MakeGame({{{1, 4}, kRA}, {{0, 4}, kRG}, {{9, 3}, kBG}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(1, 4));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(0, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(0, 5)));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(2, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(1, 4), Idx(2, 5)));
  EXPECT_EQ(moves.size(), 4u);
}

TEST(GameAction, FR_ADV_PALACE_AdvisorCannotLeavePalace) {
  // Advisor at corner (0, 3): only diagonal target is (1, 4).
  const XqGame game =
      MakeGame({{{0, 3}, kRA}, {{0, 4}, kRG}, {{9, 3}, kBG}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 3));
  EXPECT_TRUE(HasMove(moves, Idx(0, 3), Idx(1, 4)));
  EXPECT_EQ(moves.size(), 1u);
}

// --------------------- Elephant (FR-ELE-*) --------------------------------

TEST(GameAction, FR_ELE_DIAG_2_ElephantTwoStepDiagonal) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{2, 4}, kRE}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(2, 4));
  EXPECT_TRUE(HasMove(moves, Idx(2, 4), Idx(0, 2)));
  EXPECT_TRUE(HasMove(moves, Idx(2, 4), Idx(0, 6)));
  EXPECT_TRUE(HasMove(moves, Idx(2, 4), Idx(4, 2)));
  EXPECT_TRUE(HasMove(moves, Idx(2, 4), Idx(4, 6)));
  EXPECT_EQ(moves.size(), 4u);
}

TEST(GameAction, FR_ELE_NO_RIVER_RedElephantStaysBelowFive) {
  // Red Elephant at (4, 4) — moving diag-2 to row 6 would cross river.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 4}, kRE}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  for (const XqA& a : moves) {
    const uint8_t to_r = a.to / kBoardCols;
    EXPECT_LE(to_r, 4u) << "Red Elephant must not cross the river.";
  }
  // Should still have moves to (2, 2) and (2, 6) (eyes are empty).
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(2, 2)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(2, 6)));
}

TEST(GameAction, FR_ELE_EYE_BLOCKED_ElephantBlockedByMidpoint) {
  // Eye at (1, 5) is occupied by a friendly piece; Elephant at (0, 6)
  // cannot reach (2, 4) via that diagonal, but other 2-diag targets
  // (where eye is empty) are still allowed.
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{0, 6}, kRE}, {{1, 5}, kRH}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 6));
  EXPECT_FALSE(HasMove(moves, Idx(0, 6), Idx(2, 4)));
  // (2, 8) is a legitimate target if (1, 7) is empty.
  EXPECT_TRUE(HasMove(moves, Idx(0, 6), Idx(2, 8)));
}

// --------------------- Horse (FR-HORSE-*) --------------------------------

TEST(GameAction, FR_HORSE_L_HorseFullEightTargets) {
  // Horse on (4, 4) in a clear board has all 8 L-moves.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 4}, kRH}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(6, 5)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(6, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(2, 5)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(2, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(5, 6)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(5, 2)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(3, 6)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(3, 2)));
  EXPECT_EQ(moves.size(), 8u);
}

TEST(GameAction, FR_HORSE_LEG_BLOCKED_HorseHobbled) {
  // Horse at (4, 4) with a friendly piece on (5, 4) — blocks the
  // (6, 5) and (6, 3) moves but not the others.
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 4}, kRH}, {{5, 4}, kRP}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  EXPECT_FALSE(HasMove(moves, Idx(4, 4), Idx(6, 5)));
  EXPECT_FALSE(HasMove(moves, Idx(4, 4), Idx(6, 3)));
  // Other legs still free.
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(2, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(2, 5)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(5, 6)));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(3, 2)));
}

// --------------------- Chariot (FR-CHAR-*) --------------------------------

TEST(GameAction, FR_CHAR_LINE_ChariotSlidesAlongFile) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 0}, kRC}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  // Chariot on (4, 0): file moves to rows {0, 1, 2, 3, 5, 6, 7, 8, 9}
  // (excluding (4, 0) itself and (0, 4)/(9, 4) on col 4).
  // But also rank moves to cols {1..8}.
  for (uint8_t r = 0; r < kBoardRows; ++r) {
    if (r == 4) continue;
    EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(r, 0)));
  }
  for (uint8_t c = 1; c < kBoardCols; ++c) {
    EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(4, c)));
  }
}

TEST(GameAction, FR_CHAR_NO_JUMP_ChariotStopsAtFirstPiece) {
  // Friendly chariot blocks at (4, 4) — Chariot at (4, 0) can reach
  // (4, 1)..(4, 3) but not (4, 4) or beyond.
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 0}, kBG}, {{4, 0}, kRC}, {{4, 4}, kRP}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  for (uint8_t c = 1; c <= 3; ++c) {
    EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(4, c)));
  }
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 4)));
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
}

TEST(GameAction, FR_CHAR_CAPTURE_ChariotCapturesEnemy) {
  // Enemy at (4, 5). Chariot can reach (4, 1)..(4, 5).
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 0}, kBG}, {{4, 0}, kRC}, {{4, 5}, kBS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 6)));
}

TEST(GameAction, FR_CAPTURE_NO_FRIEND_ChariotCannotCaptureFriend) {
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 0}, kBG}, {{4, 0}, kRC}, {{4, 5}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
}

// --------------------- Cannon (FR-CANNON-*) -------------------------------

TEST(GameAction, FR_CANNON_MOVE_NoCaptureBehavesLikeChariot) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 0}, kBG}, {{4, 0}, kRP}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  // No screens => no captures, but still free movement along open
  // ranks/files.
  for (uint8_t c = 1; c < kBoardCols; ++c) {
    EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(4, c)));
  }
}

TEST(GameAction, FR_CANNON_NEED_SCREEN_NoCaptureWithoutScreen) {
  // Cannon at (4, 0), enemy at (4, 5), no screen between.
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 0}, kRP}, {{4, 5}, kBS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  // Cannon can move to (4, 1)..(4, 4) (empty cells), but capture at
  // (4, 5) needs a screen.
  EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(4, 4)));
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
}

TEST(GameAction, FR_CANNON_ONE_SCREEN_CaptureWithExactlyOneScreen) {
  // Cannon at (4, 0), screen at (4, 2), enemy at (4, 5).
  const XqGame game = MakeGame({{{0, 4}, kRG},
                                {{9, 3}, kBG},
                                {{4, 0}, kRP},
                                {{4, 2}, kRS},
                                {{4, 5}, kBS}},
                               kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  EXPECT_TRUE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
  // Cannot capture beyond first enemy.
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 6)));
  // Cannot move past the screen non-capturing.
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 3)));
}

TEST(GameAction, FR_CANNON_ONE_SCREEN_TwoScreensBlockCapture) {
  // Cannon at (4, 0), two screens at (4, 2) and (4, 3), enemy at (4, 5).
  const XqGame game = MakeGame({{{0, 4}, kRG},
                                {{9, 3}, kBG},
                                {{4, 0}, kRP},
                                {{4, 2}, kRS},
                                {{4, 3}, kRS},
                                {{4, 5}, kBS}},
                               kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
}

TEST(GameAction, FR_CANNON_TARGET_ENEMY_FriendlyTargetRejected) {
  // Cannon at (4, 0), screen at (4, 2), friendly at (4, 5).
  const XqGame game = MakeGame({{{0, 4}, kRG},
                                {{9, 3}, kBG},
                                {{4, 0}, kRP},
                                {{4, 2}, kRS},
                                {{4, 5}, kRS}},
                               kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 0));
  EXPECT_FALSE(HasMove(moves, Idx(4, 0), Idx(4, 5)));
}

// --------------------- Soldier (FR-SOLDIER-*) -----------------------------

TEST(GameAction, FR_SOLDIER_FORWARD_PRE_RedSoldierBeforeRiver) {
  // Red Soldier at (3, 4), still on Red's side.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{3, 4}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(3, 4));
  EXPECT_TRUE(HasMove(moves, Idx(3, 4), Idx(4, 4)));
  EXPECT_EQ(moves.size(), 1u);
}

TEST(GameAction, FR_SOLDIER_SIDEWAYS_POST_RedSoldierAfterRiver) {
  // Red Soldier at (5, 4) — across the river.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{5, 4}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(5, 4));
  EXPECT_TRUE(HasMove(moves, Idx(5, 4), Idx(6, 4)));  // forward
  EXPECT_TRUE(HasMove(moves, Idx(5, 4), Idx(5, 3)));  // left
  EXPECT_TRUE(HasMove(moves, Idx(5, 4), Idx(5, 5)));  // right
  EXPECT_EQ(moves.size(), 3u);
}

TEST(GameAction, FR_SOLDIER_NO_BACK_RedSoldierCannotMoveBackward) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{5, 4}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(5, 4));
  EXPECT_FALSE(HasMove(moves, Idx(5, 4), Idx(4, 4)));
}

TEST(GameAction, FR_SOLDIER_FORWARD_PRE_BlackSoldierBeforeRiver) {
  // Black Soldier at (6, 4) — still on Black's side.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{6, 4}, kBS}}, kBlack);
  const auto moves = MovesFrom(ValidActions(game), Idx(6, 4));
  EXPECT_TRUE(HasMove(moves, Idx(6, 4), Idx(5, 4)));
  EXPECT_EQ(moves.size(), 1u);
}

TEST(GameAction, FR_SOLDIER_SIDEWAYS_POST_BlackSoldierAfterRiver) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 4}, kBS}}, kBlack);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(3, 4)));  // forward
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(4, 3)));  // left
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(4, 5)));  // right
  EXPECT_EQ(moves.size(), 3u);
}

TEST(GameAction, FR_SOLDIER_NO_PROMOTION_StaysSoldierAtBackRank) {
  // Red Soldier at row 9 (Black's back rank); should still only have
  // sideways moves (it cannot move forward off the board).
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{8, 0}, kBG}, {{9, 4}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(9, 4));
  EXPECT_TRUE(HasMove(moves, Idx(9, 4), Idx(9, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(9, 4), Idx(9, 5)));
  EXPECT_FALSE(HasMove(moves, Idx(9, 4), Idx(8, 4)));  // backward
  EXPECT_EQ(moves.size(), 2u);
}

// --------------------- Apply / Undo round trips ---------------------------

TEST(GameAction, ApplyMutatesBoardForFromAndToCells) {
  XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  ASSERT_FALSE(actions.empty());
  const XqA a = actions.front();
  game.ApplyActionInPlace(a);
  EXPECT_EQ(game.GetBoard()[a.from], 0)
      << "Source cell must be empty after the move.";
  EXPECT_NE(game.GetBoard()[a.to], 0)
      << "Destination cell must be occupied by the moving piece.";
}

TEST(GameAction, FR_MCTS_APPLY_UNDO_SingleStepRestoresState) {
  XqGame game;
  const XqB before_board = game.GetBoard();
  const uint32_t before_round = game.CurrentRound();
  const XqP before_player = game.CurrentPlayer();
  const std::vector<XqA> actions = ValidActions(game);
  ASSERT_FALSE(actions.empty());
  game.ApplyActionInPlace(actions.front());
  game.UndoLastAction();
  EXPECT_EQ(game.GetBoard(), before_board);
  EXPECT_EQ(game.CurrentRound(), before_round);
  EXPECT_EQ(game.CurrentPlayer(), before_player);
  EXPECT_FALSE(game.LastAction().has_value());
  EXPECT_FALSE(game.LastPlayer().has_value());
}

TEST(GameAction, FR_MCTS_DEEP_ROLLOUT_DeepUndoRestoresState) {
  XqGame game;
  const XqB before_board = game.GetBoard();
  const uint32_t before_round = game.CurrentRound();
  const XqP before_player = game.CurrentPlayer();

  std::vector<XqA> applied;
  // Roll out 50 plies with the first valid action at each step.
  for (int i = 0; i < 50; ++i) {
    const std::vector<XqA> actions = ValidActions(game);
    if (actions.empty()) break;
    applied.push_back(actions.front());
    game.ApplyActionInPlace(actions.front());
  }
  // Undo all of them.
  for (size_t i = 0; i < applied.size(); ++i) {
    game.UndoLastAction();
  }
  EXPECT_EQ(game.GetBoard(), before_board);
  EXPECT_EQ(game.CurrentRound(), before_round);
  EXPECT_EQ(game.CurrentPlayer(), before_player);
}

TEST(GameAction, FR_MCTS_DEEP_ROLLOUT_FullCapacityRoundTrip) {
  // Roll out the MAXIMUM rollout depth the engine will encounter
  // (== *kMaxRounds), then undo back to the start.
  XqGame game;
  const XqB before_board = game.GetBoard();
  const uint32_t before_round = game.CurrentRound();
  const XqP before_player = game.CurrentPlayer();

  std::vector<XqA> applied;
  applied.reserve(*XqGame::kMaxRounds);
  while (applied.size() < *XqGame::kMaxRounds) {
    if (game.IsOver()) break;
    const std::vector<XqA> actions = ValidActions(game);
    if (actions.empty()) break;
    applied.push_back(actions.front());
    game.ApplyActionInPlace(actions.front());
  }
  for (size_t i = 0; i < applied.size(); ++i) {
    game.UndoLastAction();
  }
  EXPECT_EQ(game.GetBoard(), before_board);
  EXPECT_EQ(game.CurrentRound(), before_round);
  EXPECT_EQ(game.CurrentPlayer(), before_player);
}

TEST(GameAction, FR_CAPTURE_REMOVE_CapturedPieceRestoredOnUndo) {
  // Set up a position with a clean Chariot capture available, then
  // verify Apply + Undo restores the captured piece.
  XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 0}, kRC}, {{4, 5}, kBS}}, kRed);
  const XqA capture{Idx(4, 0), Idx(4, 5)};
  ASSERT_TRUE(HasMove(ValidActions(game), capture.from, capture.to));

  game.ApplyActionInPlace(capture);
  EXPECT_EQ(game.GetBoard()[Idx(4, 5)], kRC)
      << "Captured cell should now hold the moved Chariot.";
  EXPECT_EQ(game.GetBoard()[Idx(4, 0)], 0);

  game.UndoLastAction();
  EXPECT_EQ(game.GetBoard()[Idx(4, 0)], kRC);
  EXPECT_EQ(game.GetBoard()[Idx(4, 5)], kBS)
      << "Captured Black Soldier must be restored on undo.";
}

TEST(GameAction, FR_GEN_CAPTURE_GeneralCapturesAdjacentEnemy) {
  // Black Chariot has invaded Red's palace next to Red General. Red
  // General captures it.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{0, 5}, kBC}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 4));
  EXPECT_TRUE(HasMove(moves, Idx(0, 4), Idx(0, 5)));

  XqGame mut = game;
  mut.ApplyActionInPlace(XqA{Idx(0, 4), Idx(0, 5)});
  EXPECT_EQ(mut.GetBoard()[Idx(0, 4)], 0);
  EXPECT_EQ(mut.GetBoard()[Idx(0, 5)], kRG);
}

TEST(GameAction, FR_ELE_CAPTURE_ElephantCapturesEnemyAtDiagonalTwo) {
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{2, 4}, kRE}, {{4, 6}, kBS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(2, 4));
  EXPECT_TRUE(HasMove(moves, Idx(2, 4), Idx(4, 6)));
  XqGame mut = game;
  mut.ApplyActionInPlace(XqA{Idx(2, 4), Idx(4, 6)});
  EXPECT_EQ(mut.GetBoard()[Idx(4, 6)], kRE);
  EXPECT_EQ(mut.GetBoard()[Idx(2, 4)], 0);
}

TEST(GameAction, FR_HORSE_CAPTURE_HorseCapturesEnemyAtLDestination) {
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 4}, kRH}, {{6, 5}, kBS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(6, 5)));
  XqGame mut = game;
  mut.ApplyActionInPlace(XqA{Idx(4, 4), Idx(6, 5)});
  EXPECT_EQ(mut.GetBoard()[Idx(6, 5)], kRH);
  EXPECT_EQ(mut.GetBoard()[Idx(4, 4)], 0);
}

TEST(GameAction, FR_SOLDIER_NO_DIAG_PreRiverNoDiagonalMoves) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{3, 4}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(3, 4));
  EXPECT_FALSE(HasMove(moves, Idx(3, 4), Idx(4, 3)));
  EXPECT_FALSE(HasMove(moves, Idx(3, 4), Idx(4, 5)));
}

TEST(GameAction, FR_SOLDIER_NO_DIAG_PostRiverNoDiagonalMoves) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{5, 4}, kRS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(5, 4));
  EXPECT_FALSE(HasMove(moves, Idx(5, 4), Idx(6, 3)));
  EXPECT_FALSE(HasMove(moves, Idx(5, 4), Idx(6, 5)));
}

TEST(GameAction, FR_SOLDIER_CAPTURE_SoldierCapturesEnemyForward) {
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{4, 4}, kRS}, {{5, 4}, kBS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  EXPECT_TRUE(HasMove(moves, Idx(4, 4), Idx(5, 4)));
  XqGame mut = game;
  mut.ApplyActionInPlace(XqA{Idx(4, 4), Idx(5, 4)});
  EXPECT_EQ(mut.GetBoard()[Idx(5, 4)], kRS);
  EXPECT_EQ(mut.GetBoard()[Idx(4, 4)], 0);
}

TEST(GameAction, FR_CHECK_DETECTED_AttackedGeneralForcesEscape) {
  // Red General at (0, 4) under attack by Black Chariot on (5, 4).
  // The only Red piece is the General. ValidActions must contain
  // only moves that escape the check. (0, 3) and (0, 5) are not
  // attacked; (1, 4) is still on col 4 and remains in check.
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 0}, kBG}, {{5, 4}, kBC}}, kRed);
  const std::vector<XqA> actions = ValidActions(game);
  for (const XqA& a : actions) {
    const uint8_t to_col = a.to % kBoardCols;
    EXPECT_NE(to_col, 4u)
        << "A move staying on col 4 keeps the General in check.";
  }
  // The General must be able to step off col 4 to escape.
  bool any_escape = false;
  for (const XqA& a : actions) {
    if (a.from == Idx(0, 4) && (a.to == Idx(0, 3) || a.to == Idx(0, 5))) {
      any_escape = true;
      break;
    }
  }
  EXPECT_TRUE(any_escape) << "Expected at least one escape move.";
}

TEST(GameAction, FR_MOVE_NO_SELF_CHECK_RejectsMovesLeavingOwnGeneralAttacked) {
  // Red General at (1, 4); Black Chariot at (1, 0). Red Soldier at
  // (1, 2) is the only thing blocking; moving any other piece off the
  // file is fine. Try to verify the soldier cannot leave the file.
  // To do that we need the soldier to have a non-blocking option;
  // soldiers don't move sideways pre-river, so place an Advisor
  // instead at (1, 4) -> wait, that's the general's square.
  // Instead, place a Red Chariot at (1, 4) blocking, with the Red
  // General at (0, 4). The Chariot is then pinned along col 4.
  // Black has a Chariot at (9, 4) shooting down col 4. The Red
  // Chariot must not be allowed to leave col 4.
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{8, 0}, kBG}, {{1, 4}, kRC}, {{9, 4}, kBC}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(1, 4));
  for (const XqA& a : moves) {
    EXPECT_EQ(a.to % kBoardCols, 4u)
        << "Pinned piece must not leave col 4 (would expose own General).";
  }
}

// ----------------------- Additional edge-case coverage -------------------

TEST(GameAction, BlackGeneralOrthogonalMovesInsidePalace) {
  const XqGame game = MakeGame({{{0, 0}, kRG}, {{8, 4}, kBG}}, kBlack);
  const auto moves = MovesFrom(ValidActions(game), Idx(8, 4));
  EXPECT_TRUE(HasMove(moves, Idx(8, 4), Idx(7, 4)));
  EXPECT_TRUE(HasMove(moves, Idx(8, 4), Idx(9, 4)));
  EXPECT_TRUE(HasMove(moves, Idx(8, 4), Idx(8, 3)));
  EXPECT_TRUE(HasMove(moves, Idx(8, 4), Idx(8, 5)));
  EXPECT_EQ(moves.size(), 4u);
}

TEST(GameAction, BlackAdvisorReachesPalaceCenter) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 4}, kBG}, {{9, 3}, kBA}}, kBlack);
  const auto moves = MovesFrom(ValidActions(game), Idx(9, 3));
  EXPECT_TRUE(HasMove(moves, Idx(9, 3), Idx(8, 4)));
  EXPECT_EQ(moves.size(), 1u);
}

TEST(GameAction, BlackElephantStaysOnOwnSide) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{5, 4}, kBE}}, kBlack);
  const auto moves = MovesFrom(ValidActions(game), Idx(5, 4));
  for (const XqA& a : moves) {
    const uint8_t to_r = a.to / kBoardCols;
    EXPECT_GE(to_r, 5u) << "Black Elephant must stay on rows 5-9.";
  }
}

TEST(GameAction, ElephantBlockedByEnemyInEye) {
  // Eye blocked by an ENEMY piece (not just friendly).
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{0, 6}, kRE}, {{1, 5}, kBS}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 6));
  EXPECT_FALSE(HasMove(moves, Idx(0, 6), Idx(2, 4)));
}

TEST(GameAction, HorseHobbledInAllFourLegDirections) {
  // Place a Horse at (4, 4) and 4 friendly soldiers at the orthogonal
  // cells (3, 4), (5, 4), (4, 3), (4, 5). Every L-move's leg is
  // blocked, so 0 legal horse moves.
  const XqGame game = MakeGame({{{0, 4}, kRG},
                                {{9, 3}, kBG},
                                {{4, 4}, kRH},
                                {{3, 4}, kRS},
                                {{5, 4}, kRS},
                                {{4, 3}, kRS},
                                {{4, 5}, kRS}},
                               kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(4, 4));
  EXPECT_EQ(moves.size(), 0u)
      << "Horse with all four legs blocked has zero L-moves.";
}

TEST(GameAction, HorseAtBoardCornerHasOnlyTwoMoves) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{0, 0}, kRH}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 0));
  EXPECT_TRUE(HasMove(moves, Idx(0, 0), Idx(1, 2)));
  EXPECT_TRUE(HasMove(moves, Idx(0, 0), Idx(2, 1)));
  EXPECT_EQ(moves.size(), 2u);
}

TEST(GameAction, ChariotAtBoardCornerSweepsTwoLines) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{0, 0}, kRC}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 0));
  // Should reach (0, 1)..(0, 8) except (0, 4) where Red General sits.
  for (uint8_t c = 1; c <= 3; ++c) {
    EXPECT_TRUE(HasMove(moves, Idx(0, 0), Idx(0, c)));
  }
  EXPECT_FALSE(HasMove(moves, Idx(0, 0), Idx(0, 4)))
      << "Cannot capture own General.";
  EXPECT_FALSE(HasMove(moves, Idx(0, 0), Idx(0, 5)))
      << "Cannot pass through own General.";
  // Should also reach (1, 0)..(9, 0).
  for (uint8_t r = 1; r < kBoardRows; ++r) {
    EXPECT_TRUE(HasMove(moves, Idx(0, 0), Idx(r, 0)));
  }
}

TEST(GameAction, CannonCanShootAcrossManyEmptyCells) {
  // Cannon at (0, 0), screen at (5, 0), enemy at (9, 0). Cannon
  // must reach (9, 0) by jumping over the screen.
  const XqGame game = MakeGame({{{0, 4}, kRG},
                                {{9, 3}, kBG},
                                {{0, 0}, kRP},
                                {{5, 0}, kRS},
                                {{9, 0}, kBS}},
                               kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(0, 0));
  EXPECT_TRUE(HasMove(moves, Idx(0, 0), Idx(9, 0)));
}

TEST(GameAction, BlackSoldierForwardIsNegativeRowDelta) {
  const XqGame game =
      MakeGame({{{0, 4}, kRG}, {{9, 3}, kBG}, {{6, 0}, kBS}}, kBlack);
  const auto moves = MovesFrom(ValidActions(game), Idx(6, 0));
  EXPECT_TRUE(HasMove(moves, Idx(6, 0), Idx(5, 0)));
  EXPECT_FALSE(HasMove(moves, Idx(6, 0), Idx(7, 0)))
      << "Black Soldier never moves toward higher rows.";
}

TEST(GameAction, RepetitionOnlyBetweenIdenticalPositions) {
  // Construct a position where Red's chariot can shuttle b3<->b4 while
  // Black's chariot does the same. After 4 plies the position recurs;
  // after 8 plies it's threefold.
  XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{3, 1}, kRC}, {{6, 1}, kBC}}, kRed);
  ASSERT_FALSE(game.IsOver());
  const XqA red_up{Idx(3, 1), Idx(4, 1)};
  const XqA red_dn{Idx(4, 1), Idx(3, 1)};
  const XqA blk_up{Idx(6, 1), Idx(5, 1)};
  const XqA blk_dn{Idx(5, 1), Idx(6, 1)};

  game.ApplyActionInPlace(red_up);
  game.ApplyActionInPlace(blk_up);
  game.ApplyActionInPlace(red_dn);
  game.ApplyActionInPlace(blk_dn);
  // Position recurred once (count=2 with current).
  EXPECT_FALSE(game.IsOver());
  game.ApplyActionInPlace(red_up);
  game.ApplyActionInPlace(blk_up);
  game.ApplyActionInPlace(red_dn);
  game.ApplyActionInPlace(blk_dn);
  // Now at threefold.
  EXPECT_TRUE(game.IsOver());
  EXPECT_FLOAT_EQ(game.GetScore(kRed), 0.0F);
  EXPECT_FLOAT_EQ(game.GetScore(kBlack), 0.0F);
}

TEST(GameAction, UndoBeyondZeroIsNoOp) {
  XqGame game;
  EXPECT_EQ(game.CurrentRound(), 0u);
  game.UndoLastAction();
  EXPECT_EQ(game.CurrentRound(), 0u);
  EXPECT_EQ(game.CurrentPlayer(), kRed);
}

TEST(GameAction, ApplyAndUndoPreservePositionHashViaIsOverConsistency) {
  // After Apply -> Undo, the game should not become "Over" if it
  // wasn't before. This catches Zobrist-update bugs that would create
  // a fake threefold repetition after undo.
  XqGame game;
  for (int i = 0; i < 5; ++i) {
    const auto actions = ValidActions(game);
    if (actions.empty()) break;
    game.ApplyActionInPlace(actions.front());
    EXPECT_FALSE(game.IsOver());
    game.UndoLastAction();
    EXPECT_FALSE(game.IsOver());
  }
}

TEST(GameAction, SnapshotConstructorPreservesPositionHashViaIsOver) {
  // Snapshot from initial position should have the same IsOver status
  // and ValidActions count as a fresh game.
  const XqGame fresh;
  const XqGame snap(fresh.GetBoard(), fresh.CurrentPlayer(), 0, std::nullopt);
  EXPECT_EQ(snap.IsOver(), fresh.IsOver());
  EXPECT_EQ(ValidActions(snap).size(), ValidActions(fresh).size());
}

TEST(GameAction, EveryValidActionAppliedAndUndoneRestoresState) {
  // For the initial position, apply each valid action then undo it
  // and verify the board is restored.
  XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  ASSERT_FALSE(actions.empty());
  const XqB initial_board = game.GetBoard();
  for (const XqA& a : actions) {
    game.ApplyActionInPlace(a);
    game.UndoLastAction();
    EXPECT_EQ(game.GetBoard(), initial_board)
        << "Apply+Undo of (" << static_cast<int>(a.from) << ", "
        << static_cast<int>(a.to) << ") corrupted the board.";
    EXPECT_EQ(game.CurrentRound(), 0u);
    EXPECT_EQ(game.CurrentPlayer(), kRed);
  }
}

TEST(GameAction, CheckEscapableByCapturingTheChecker) {
  // Red General at (1, 4). Black Chariot at (3, 4) attacks col 4 →
  // Red is in check. Red Chariot at (3, 5) can capture the checker
  // by moving one step left to (3, 4).
  const XqGame game = MakeGame(
      {{{1, 4}, kRG}, {{9, 0}, kBG}, {{3, 5}, kRC}, {{3, 4}, kBC}}, kRed);
  const std::vector<XqA> actions = ValidActions(game);
  EXPECT_TRUE(HasMove(actions, Idx(3, 5), Idx(3, 4)))
      << "Capturing the checker is a legal way to escape check.";
}

TEST(GameAction, MoveExposingDiscoveredSelfCheckIsRejected) {
  // Red General at (0, 4). Red Cannon at (3, 4). Black Chariot at
  // (5, 4) — currently the cannon screens for the chariot AT THE
  // CHARIOT itself (not relevant). The cannon is what matters; with
  // the Black chariot at (5, 4), removing the cannon at (3, 4)
  // exposes the General to the chariot. So moving the cannon off
  // col 4 is illegal.
  const XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{3, 4}, kRP}, {{5, 4}, kBC}}, kRed);
  const std::vector<XqA> moves = MovesFrom(ValidActions(game), Idx(3, 4));
  for (const XqA& a : moves) {
    EXPECT_EQ(a.to % kBoardCols, 4u)
        << "Cannon must stay on col 4 to keep blocking the Chariot.";
  }
}

TEST(GameAction, RepetitionCounterResetsCorrectlyAfterDeepUndo) {
  XqGame game = MakeGame(
      {{{0, 4}, kRG}, {{9, 3}, kBG}, {{3, 1}, kRC}, {{6, 1}, kBC}}, kRed);
  const XqA red_up{Idx(3, 1), Idx(4, 1)};
  const XqA red_dn{Idx(4, 1), Idx(3, 1)};
  const XqA blk_up{Idx(6, 1), Idx(5, 1)};
  const XqA blk_dn{Idx(5, 1), Idx(6, 1)};
  // Apply the full shuttle sequence to threefold.
  game.ApplyActionInPlace(red_up);
  game.ApplyActionInPlace(blk_up);
  game.ApplyActionInPlace(red_dn);
  game.ApplyActionInPlace(blk_dn);
  game.ApplyActionInPlace(red_up);
  game.ApplyActionInPlace(blk_up);
  game.ApplyActionInPlace(red_dn);
  game.ApplyActionInPlace(blk_dn);
  ASSERT_TRUE(game.IsOver());
  // Undo all the way to the start; that position has only been seen
  // once (the snapshot) so IsOver must be false.
  for (int i = 0; i < 8; ++i) game.UndoLastAction();
  EXPECT_FALSE(game.IsOver())
      << "Full undo must restore the non-terminal starting position.";
  EXPECT_EQ(game.CurrentRound(), 0u);
}

TEST(GameAction, GeneralCannotCaptureProtectedAdjacentEnemy) {
  // Red General at (1, 4), Black Soldier at (1, 5) protected by a
  // Black Chariot at (5, 5) (covers col 5). Capturing the Soldier
  // would put the General in check from the Chariot.
  const XqGame game = MakeGame(
      {{{1, 4}, kRG}, {{9, 3}, kBG}, {{1, 5}, kBS}, {{5, 5}, kBC}}, kRed);
  const auto moves = MovesFrom(ValidActions(game), Idx(1, 4));
  EXPECT_FALSE(HasMove(moves, Idx(1, 4), Idx(1, 5)))
      << "General cannot capture into a square defended by an enemy "
         "Chariot — it would still be in check.";
}

}  // namespace
}  // namespace az::game::xq
