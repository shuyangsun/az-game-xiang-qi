#include "include/xq/serializer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "gtest/gtest.h"
#include "include/xq/game.h"
#include "tests/unit/valid_actions.h"

namespace az::game::xq {
namespace {

using ::az::game::api::RingBuffer;
using ::az::game::api::TrainingTarget;

using GameHistory =
    RingBuffer<XqGame, std::array<XqGame, XqGame::kHistoryLookback>>;

constexpr size_t kStateVectorLen =
    kDenseStateFeatureSize;  // 15 planes × 90 cells
constexpr size_t kPolicyVectorLen = XqGame::kPolicySize + 1;

// Compact-input float-index offsets used by the new transformer-oriented
// layout. The state vector is 195 tokens of width 2:
//   tokens 0..89        : board tokens   [piece_code, 0]
//   token  90           : repeat counter [repeat_count, 0]
//   tokens 91..194      : action slots   [from, to]
constexpr size_t kCompactBoardRowOffset =
    kCompactBoardTokenOffset * kCompactTokenFeatureWidth;
constexpr size_t kCompactRepeatCounterOffset =
    kCompactRepeatTokenOffset * kCompactTokenFeatureWidth;
constexpr size_t kCompactActionRowOffset =
    kCompactActionTokenOffset * kCompactTokenFeatureWidth;

struct ExpectedCompactAction {
  size_t policy_index;
  size_t valid_action_index;
  float from_feature;
  float to_feature;
};

bool ExpectedCompactActionLess(const ExpectedCompactAction& lhs,
                               const ExpectedCompactAction& rhs) noexcept {
  return lhs.policy_index < rhs.policy_index;
}

std::vector<ExpectedCompactAction> ExpectedCompactActions(const XqGame& game) {
  const std::vector<XqA> actions = ValidActions(game);
  std::vector<ExpectedCompactAction> expected;
  expected.reserve(actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    const XqA canonical = game.CanonicalAction(actions[i]);
    expected.push_back(ExpectedCompactAction{
        game.PolicyIndex(canonical), i, static_cast<float>(canonical.from),
        static_cast<float>(canonical.to)});
  }
  std::sort(expected.begin(), expected.end(), ExpectedCompactActionLess);
  return expected;
}

// Find a legal move matching `(from, to)` in `game`. Returns the
// no-action sentinel if no such legal move exists; the caller is
// expected to assert before applying it.
[[nodiscard]] XqA FindLegalMove(const XqGame& game, uint8_t from,
                                uint8_t to) noexcept {
  std::array<XqA, XqGame::kMaxLegalActions> buf{};
  const size_t n = game.ValidActionsInto(buf);
  for (size_t i = 0; i < n; ++i) {
    if (buf[i].from == from && buf[i].to == to) return buf[i];
  }
  return XqA{kBoardCells, kBoardCells};
}

TEST(Serializer, FR_SER_FIXED_LEN_CurrentStateInitial) {
  const XqGame game;
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> nn_in =
      serializer.SerializeCurrentState(game, history.View());
  EXPECT_EQ(nn_in.size(), kStateVectorLen);
}

TEST(Serializer, FR_SER_INPUT_LEN_AfterApply) {
  XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  game.ApplyActionInPlace(actions.front());
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> nn_in =
      serializer.SerializeCurrentState(game, history.View());
  EXPECT_EQ(nn_in.size(), kStateVectorLen);
}

TEST(Serializer, FR_SER_EMPTY_HISTORY_AcceptsEmptyView) {
  // Markov game: empty history must work.
  const XqGame game;
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> nn_in =
      serializer.SerializeCurrentState(game, history.View());
  EXPECT_EQ(nn_in.size(), kStateVectorLen);
}

TEST(Serializer, FR_SER_ENCODES_PLAYER_PlanesDifferAcrossPlayers) {
  const XqGame red_to_move;
  const XqGame black_to_move(kBlack);
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> red_in =
      serializer.SerializeCurrentState(red_to_move, history.View());
  const std::vector<float> black_in =
      serializer.SerializeCurrentState(black_to_move, history.View());
  ASSERT_EQ(red_in.size(), black_in.size());
  // The two encodings must differ — at minimum, the side-to-move plane.
  bool any_difference = false;
  for (size_t i = 0; i < red_in.size(); ++i) {
    if (red_in[i] != black_in[i]) {
      any_difference = true;
      break;
    }
  }
  EXPECT_TRUE(any_difference)
      << "Encoding for Red-to-move and Black-to-move must differ.";
}

TEST(Serializer, FR_SER_POLICY_LEN_DefaultGame) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi(actions.size(),
                        1.0F / static_cast<float>(actions.size()));
  const TrainingTarget target{0.25F, std::move(pi)};
  const XqSerializer serializer;
  const std::vector<float> out = serializer.SerializePolicyOutput(game, target);
  EXPECT_EQ(out.size(), kPolicyVectorLen);
}

TEST(Serializer, FR_SER_POLICY_Z_FIRST_PutsValueInSlotZero) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi(actions.size(),
                        1.0F / static_cast<float>(actions.size()));
  const TrainingTarget target{0.42F, std::move(pi)};
  const XqSerializer serializer;
  const std::vector<float> out = serializer.SerializePolicyOutput(game, target);
  ASSERT_EQ(out.size(), kPolicyVectorLen);
  EXPECT_FLOAT_EQ(out[0], 0.42F);
}

TEST(Serializer, FR_SER_POLICY_MASK_InvalidActionsZero) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi(actions.size(),
                        1.0F / static_cast<float>(actions.size()));
  const TrainingTarget target{0.0F, std::move(pi)};
  const XqSerializer serializer;
  const std::vector<float> out = serializer.SerializePolicyOutput(game, target);
  ASSERT_EQ(out.size(), kPolicyVectorLen);

  // Build the set of valid policy indices.
  std::vector<bool> valid(XqGame::kPolicySize, false);
  for (const XqA& a : actions) {
    valid[game.PolicyIndex(a)] = true;
  }
  for (size_t i = 0; i < XqGame::kPolicySize; ++i) {
    if (!valid[i]) {
      EXPECT_FLOAT_EQ(out[1 + i], 0.0F)
          << "Invalid action slot " << i << " must be zero, got " << out[1 + i];
    }
  }
}

TEST(Serializer, FR_SER_POLICY_MASK_ValidActionsScattered) {
  const XqGame game;
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  // Use distinct probabilities so we can verify the scatter location.
  std::vector<float> pi;
  pi.reserve(actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    pi.push_back(static_cast<float>(i + 1));
  }
  const TrainingTarget target{0.0F, pi};
  const XqSerializer serializer;
  const std::vector<float> out = serializer.SerializePolicyOutput(game, target);
  for (size_t i = 0; i < actions.size(); ++i) {
    const size_t slot = 1 + game.PolicyIndex(game.CanonicalAction(actions[i]));
    EXPECT_FLOAT_EQ(out[slot], pi[i]) << "Mismatch at slot " << slot;
  }
}

// The slot indexing must use the canonical frame: for Black to move,
// scattered probabilities must land at `PolicyIndex(CanonicalAction(a))`,
// NOT at `PolicyIndex(a)` (those would correspond to live-frame
// indexing and break Red↔Black weight sharing in the policy head).
TEST(Serializer, FR_SER_POLICY_SLOTS_AreCanonicalFrameForBlack) {
  const XqGame game(kBlack);
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi;
  pi.reserve(actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    pi.push_back(static_cast<float>(i + 1));
  }
  const TrainingTarget target{0.0F, pi};
  const XqSerializer serializer;
  const std::vector<float> out = serializer.SerializePolicyOutput(game, target);

  for (size_t i = 0; i < actions.size(); ++i) {
    const size_t canonical_slot =
        1 + game.PolicyIndex(game.CanonicalAction(actions[i]));
    const size_t live_slot = 1 + game.PolicyIndex(actions[i]);
    EXPECT_FLOAT_EQ(out[canonical_slot], pi[i])
        << "Black probability not at canonical-frame slot " << canonical_slot;
    // Live-frame slot would be wrong here. Only require strict inequality
    // when the two slots actually differ (i.e., not a palindromic action
    // that maps to itself under 180-degree rotation).
    if (canonical_slot != live_slot) {
      EXPECT_FLOAT_EQ(out[live_slot], 0.0F)
          << "Black probability leaked into live-frame slot " << live_slot;
    }
  }
}

TEST(CompactSerializer, FR_SER_COMPACT_MAX_LEGAL_ACTIONS_IsTightBound) {
  EXPECT_EQ(XqGame::kMaxLegalActions, static_cast<size_t>(104));
}

TEST(CompactSerializer, FR_SER_COMPACT_INPUT_LEN_IsFixed390) {
  const XqGame game;
  const GameHistory history;
  const XqCompactSerializer serializer;
  const std::vector<float> compact =
      serializer.SerializeCurrentState(game, history.View());
  // 195 tokens of width 2 = 390 floats: 90 board tokens [piece, 0] +
  // 1 repeat token [count, 0] + 104 action tokens [from, to].
  ASSERT_EQ(kCompactStateTokenCount,
            kBoardCells + 1 + XqGame::kMaxLegalActions);
  ASSERT_EQ(kCompactStateFeatureSize,
            kCompactStateTokenCount * kCompactTokenFeatureWidth);
  EXPECT_EQ(compact.size(), kCompactStateFeatureSize);
}

TEST(CompactSerializer, FR_SER_COMPACT_BOARD_TokensAreCanonicalSignedCodes) {
  const XqGame game(kBlack);  // exercise CanonicalBoard's rotation
  const GameHistory history;
  const XqCompactSerializer serializer;
  const std::vector<float> compact =
      serializer.SerializeCurrentState(game, history.View());

  const XqB canonical = game.CanonicalBoard();
  ASSERT_EQ(compact.size(), kCompactStateFeatureSize);
  for (size_t cell = 0; cell < kBoardCells; ++cell) {
    const size_t slot =
        kCompactBoardRowOffset + cell * kCompactTokenFeatureWidth;
    EXPECT_FLOAT_EQ(compact[slot], static_cast<float>(canonical[cell]))
        << "Board token feature 0 at cell " << cell
        << " must equal the canonical signed piece code.";
    EXPECT_FLOAT_EQ(compact[slot + 1], 0.0F)
        << "Board token feature 1 at cell " << cell << " must be the zero pad.";
  }
}

TEST(CompactSerializer, FR_SER_COMPACT_REPEAT_StartsAtOne) {
  const XqGame game;
  const GameHistory history;
  const XqCompactSerializer serializer;
  const std::vector<float> compact =
      serializer.SerializeCurrentState(game, history.View());
  ASSERT_EQ(compact.size(), kCompactStateFeatureSize);
  EXPECT_FLOAT_EQ(compact[kCompactRepeatCounterOffset], 1.0F);
  EXPECT_FLOAT_EQ(compact[kCompactRepeatCounterOffset + 1], 0.0F)
      << "Repeat-counter token feature 1 must be the zero pad.";
}

TEST(CompactSerializer, FR_SER_COMPACT_REPEAT_IncrementsOnReturnToStart) {
  // Walk a back-and-forth chariot sequence so the starting position
  // is reached a second time. CurrentPositionRepeatCount() — and
  // therefore the compact repeat-counter feature — should report 2.
  XqGame game;
  const XqA red_forward = FindLegalMove(game, 0, 9);
  ASSERT_NE(red_forward.from, kBoardCells)
      << "Red chariot (0,0)->(1,0) expected legal at start.";
  game.ApplyActionInPlace(red_forward);

  const XqA black_forward = FindLegalMove(game, 81, 72);
  ASSERT_NE(black_forward.from, kBoardCells)
      << "Black chariot (9,0)->(8,0) expected legal after Red's first move.";
  game.ApplyActionInPlace(black_forward);

  const XqA red_back = FindLegalMove(game, 9, 0);
  ASSERT_NE(red_back.from, kBoardCells)
      << "Red chariot (1,0)->(0,0) expected legal.";
  game.ApplyActionInPlace(red_back);

  const XqA black_back = FindLegalMove(game, 72, 81);
  ASSERT_NE(black_back.from, kBoardCells)
      << "Black chariot (8,0)->(9,0) expected legal.";
  game.ApplyActionInPlace(black_back);

  const GameHistory history;
  const XqCompactSerializer serializer;
  const std::vector<float> compact =
      serializer.SerializeCurrentState(game, history.View());
  ASSERT_EQ(compact.size(), kCompactStateFeatureSize);
  EXPECT_FLOAT_EQ(compact[kCompactRepeatCounterOffset], 2.0F);
  EXPECT_EQ(game.CurrentPositionRepeatCount(), 2);
}

TEST(CompactSerializer, FR_SER_COMPACT_ACTIONS_SortedByCanonicalFromTo) {
  const XqGame game(kBlack);
  const GameHistory history;
  const XqCompactSerializer serializer;
  const std::vector<float> compact =
      serializer.SerializeCurrentState(game, history.View());
  const std::vector<ExpectedCompactAction> expected =
      ExpectedCompactActions(game);
  if (expected.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }

  ASSERT_EQ(compact.size(), kCompactStateFeatureSize);
  const float pad = static_cast<float>(kBoardCells);
  for (size_t i = 0; i < expected.size(); ++i) {
    const size_t slot =
        kCompactActionRowOffset + i * kCompactTokenFeatureWidth;
    EXPECT_FLOAT_EQ(compact[slot], expected[i].from_feature);
    EXPECT_FLOAT_EQ(compact[slot + 1], expected[i].to_feature);
    if (i > 0) {
      EXPECT_LT(expected[i - 1].policy_index, expected[i].policy_index);
    }
  }
  for (size_t i = expected.size(); i < XqGame::kMaxLegalActions; ++i) {
    const size_t slot =
        kCompactActionRowOffset + i * kCompactTokenFeatureWidth;
    EXPECT_FLOAT_EQ(compact[slot], pad);
    EXPECT_FLOAT_EQ(compact[slot + 1], pad);
  }
}

TEST(CompactSerializer, FR_SER_COMPACT_POLICY_PadsSortedTargets) {
  const XqGame game(kBlack);
  const std::vector<XqA> actions = ValidActions(game);
  if (actions.empty()) {
    GTEST_SKIP() << "ValidActions placeholder still empty; revisit once "
                    "GAME-ACTION-IMPL is in.";
  }
  std::vector<float> pi;
  pi.reserve(actions.size());
  for (size_t i = 0; i < actions.size(); ++i) {
    pi.push_back(static_cast<float>(i + 1));
  }

  const TrainingTarget target{0.5F, pi};
  const XqCompactSerializer serializer;
  const ::az::game::api::CompactPolicyTargetBlob blob =
      serializer.SerializePolicyOutput(game, target);
  const std::vector<ExpectedCompactAction> expected =
      ExpectedCompactActions(game);

  EXPECT_FLOAT_EQ(blob.value, target.z);
  EXPECT_EQ(blob.count, expected.size());
  ASSERT_EQ(blob.legal_indices.size(), XqGame::kMaxLegalActions);
  ASSERT_EQ(blob.values.size(), XqGame::kMaxLegalActions);

  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(blob.legal_indices[i], expected[i].policy_index);
    EXPECT_FLOAT_EQ(blob.values[i], pi[expected[i].valid_action_index]);
  }
  for (size_t i = expected.size(); i < XqGame::kMaxLegalActions; ++i) {
    EXPECT_EQ(blob.legal_indices[i],
              ::az::game::api::CompactPolicyTargetBlob::kPaddingSlot);
    EXPECT_FLOAT_EQ(blob.values[i], 0.0F);
  }
}

TEST(Serializer, OwnPiecePlanesContainOnesAtOwnPiecePositions) {
  // For Red to move on the initial position: Red Chariot at (0, 0)
  // is piece type 5 → own-piece plane index 4. The serializer is
  // built atop CanonicalBoard so own-piece codes 1..7 sit on planes
  // 0..6 (indexed by `code - 1`).
  const XqGame game;
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> nn_in =
      serializer.SerializeCurrentState(game, history.View());
  const size_t plane_chariot = 4;  // Red Chariot = code 5
  const size_t plane_size = 90;
  EXPECT_FLOAT_EQ(nn_in[plane_chariot * plane_size + 0], 1.0F);  // (0, 0)
  EXPECT_FLOAT_EQ(nn_in[plane_chariot * plane_size + 8], 1.0F);  // (0, 8)
  // Same plane should be 0 at non-chariot Red cells, e.g., (0, 4)
  // (Red General).
  EXPECT_FLOAT_EQ(nn_in[plane_chariot * plane_size + 4], 0.0F);
}

TEST(Serializer, OpponentPlanesShifted) {
  // Black Chariot in opponent slot when Red is to move: plane index
  // 7 + (5 - 1) = 11.
  const XqGame game;
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> nn_in =
      serializer.SerializeCurrentState(game, history.View());
  const size_t plane = 7 + 4;
  const size_t plane_size = 90;
  EXPECT_FLOAT_EQ(nn_in[plane * plane_size + 9 * 9 + 0], 1.0F);  // (9, 0)
  EXPECT_FLOAT_EQ(nn_in[plane * plane_size + 9 * 9 + 8], 1.0F);  // (9, 8)
  // Same plane should be 0 elsewhere (e.g., the Red Chariot squares).
  EXPECT_FLOAT_EQ(nn_in[plane * plane_size + 0], 0.0F);
}

TEST(Serializer, SideToMovePlaneIsAllOnesForRedAllZerosForBlack) {
  const XqGame red_to_move;
  const XqGame black_to_move(kBlack);
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> red =
      serializer.SerializeCurrentState(red_to_move, history.View());
  const std::vector<float> black =
      serializer.SerializeCurrentState(black_to_move, history.View());
  const size_t base = 14 * 90;
  for (size_t i = 0; i < 90; ++i) {
    EXPECT_FLOAT_EQ(red[base + i], 1.0F)
        << "Side-to-move plane should be all 1.0 for Red.";
    EXPECT_FLOAT_EQ(black[base + i], 0.0F)
        << "Side-to-move plane should be all 0.0 for Black.";
  }
}

TEST(Serializer, NoCellHasMoreThanOnePieceTypePlaneSet) {
  // For each cell, at most one of the 14 piece-type planes should be 1.
  const XqGame game;
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> nn_in =
      serializer.SerializeCurrentState(game, history.View());
  const size_t plane_size = 90;
  for (size_t cell = 0; cell < 90; ++cell) {
    int hits = 0;
    for (size_t p = 0; p < 14; ++p) {
      if (nn_in[p * plane_size + cell] == 1.0F) ++hits;
    }
    EXPECT_LE(hits, 1) << "Cell " << cell << " has multiple plane hits.";
  }
}

TEST(Serializer, ConsistentLengthOnTerminalState) {
  // Walk a long path (or until terminal) and ensure the input length
  // is stable.
  XqGame game;
  const GameHistory history;
  const XqSerializer serializer;
  const std::vector<float> initial =
      serializer.SerializeCurrentState(game, history.View());
  const size_t len = initial.size();

  for (int i = 0; i < 50; ++i) {
    if (game.IsOver()) break;
    const std::vector<XqA> actions = ValidActions(game);
    if (actions.empty()) break;
    game.ApplyActionInPlace(actions.front());
    const std::vector<float> mid =
        serializer.SerializeCurrentState(game, history.View());
    EXPECT_EQ(mid.size(), len);
  }
}

}  // namespace
}  // namespace az::game::xq
