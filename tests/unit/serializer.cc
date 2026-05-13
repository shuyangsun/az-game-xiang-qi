#include "include/xq/serializer.h"

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
    static_cast<size_t>(15) * 90;  // 15 planes × 90 cells
constexpr size_t kPolicyVectorLen = XqGame::kPolicySize + 1;

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
    // that maps to itself under vertical flip).
    if (canonical_slot != live_slot) {
      EXPECT_FLOAT_EQ(out[live_slot], 0.0F)
          << "Black probability leaked into live-frame slot " << live_slot;
    }
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
