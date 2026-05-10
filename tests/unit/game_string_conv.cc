#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include/xq/game.h"

namespace az::game::xq {
namespace {

constexpr uint8_t Idx(uint8_t row, uint8_t col) noexcept {
  return static_cast<uint8_t>(row * kBoardCols + col);
}

TEST(GameStringConv, FR_API_BOARD_STR_NONEMPTY_InitialBoardNonEmpty) {
  const XqGame game;
  EXPECT_FALSE(game.BoardReadableString().empty());
}

TEST(GameStringConv, FR_API_BOARD_STR_NONEMPTY_DistinctAcrossPositions) {
  const XqGame initial;
  XqGame moved = initial;
  const std::vector<XqA> actions = moved.ValidActions();
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  moved.ApplyActionInPlace(actions.front());
  EXPECT_NE(initial.BoardReadableString(), moved.BoardReadableString())
      << "Board strings should change after a move.";
}

TEST(GameStringConv, FR_API_ACTION_TO_STR_DETERMINISTIC_KnownExample) {
  const XqGame game;
  const XqA horse{Idx(0, 1), Idx(2, 2)};
  EXPECT_EQ(game.ActionToString(horse), "b0-c2");
}

TEST(GameStringConv, FR_API_ACTION_TO_STR_DETERMINISTIC_DistinctPerAction) {
  const XqGame game;
  std::set<std::string> seen;
  for (uint8_t f = 0; f < kBoardCells; ++f) {
    for (uint8_t t = 0; t < kBoardCells; ++t) {
      if (f == t) continue;
      const std::string s = game.ActionToString(XqA{f, t});
      EXPECT_TRUE(seen.insert(s).second)
          << "ActionToString collision on (" << static_cast<int>(f)
          << ", " << static_cast<int>(t) << "): " << s;
    }
  }
}

TEST(GameStringConv, FR_API_ACTION_ROUNDTRIP_FromKnownExample) {
  const XqGame game;
  const auto parsed = game.ActionFromString("b0-c2");
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->from, Idx(0, 1));
  EXPECT_EQ(parsed->to, Idx(2, 2));
}

TEST(GameStringConv, FR_API_ACTION_ROUNDTRIP_AllValidInitialMoves) {
  const XqGame game;
  for (const XqA& a : game.ValidActions()) {
    const std::string s = game.ActionToString(a);
    const auto parsed = game.ActionFromString(s);
    ASSERT_TRUE(parsed.has_value()) << "Failed to parse: " << s;
    EXPECT_EQ(parsed->from, a.from) << "Roundtrip mismatch on: " << s;
    EXPECT_EQ(parsed->to, a.to) << "Roundtrip mismatch on: " << s;
  }
}

TEST(GameStringConv, FR_API_ACTION_FROM_STR_ERR_EmptyInput) {
  const XqGame game;
  const auto parsed = game.ActionFromString("");
  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error(), XqError::kInvalidActionFormat);
}

TEST(GameStringConv, FR_API_ACTION_FROM_STR_ERR_GarbageInput) {
  const XqGame game;
  const auto parsed = game.ActionFromString("not-a-move");
  ASSERT_FALSE(parsed.has_value());
  EXPECT_NE(parsed.error(), XqError::kUnknownError);
}

TEST(GameStringConv, FR_API_ACTION_FROM_STR_ERR_OutOfRangeColumn) {
  const XqGame game;
  // 'z' is past 'i' (the last valid column).
  const auto parsed = game.ActionFromString("z0-c2");
  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error(), XqError::kInvalidActionFromCell);
}

TEST(GameStringConv, FR_API_ACTION_FROM_STR_ERR_OutOfRangeToColumn) {
  const XqGame game;
  const auto parsed = game.ActionFromString("a0-z2");
  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error(), XqError::kInvalidActionToCell);
}

TEST(GameStringConv, FR_API_ACTION_FROM_STR_ERR_BadFormatNoDash) {
  const XqGame game;
  const auto parsed = game.ActionFromString("b0c2");
  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error(), XqError::kInvalidActionFormat);
}

TEST(GameStringConv, ActionFromStringIsCaseInsensitive) {
  const XqGame game;
  const auto parsed_lower = game.ActionFromString("b0-c2");
  const auto parsed_upper = game.ActionFromString("B0-C2");
  ASSERT_TRUE(parsed_lower.has_value());
  ASSERT_TRUE(parsed_upper.has_value());
  EXPECT_EQ(parsed_lower->from, parsed_upper->from);
  EXPECT_EQ(parsed_lower->to, parsed_upper->to);
}

TEST(GameStringConv, ActionFromStringTrimsWhitespace) {
  const XqGame game;
  const auto parsed = game.ActionFromString("  b0-c2  ");
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->from, Idx(0, 1));
  EXPECT_EQ(parsed->to, Idx(2, 2));
}

}  // namespace
}  // namespace az::game::xq
