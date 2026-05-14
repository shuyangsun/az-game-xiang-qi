#include "include/xq/serializer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "alpha-zero-api/ring_buffer.h"
#include "include/xq/game.h"
#include "src/xq/game/internal.h"

namespace az::game::xq {

namespace {

using ::az::game::xq::internal::IsRed;

struct LegalActionFeature {
  XqA canonical_action;
  size_t policy_index;
  size_t valid_action_index;
};

constexpr size_t PlaneOffset(size_t plane) noexcept {
  return plane * kBoardCells;
}

// `XqA{kBoardCells, kBoardCells}` is the engine-wide "no action"
// sentinel (see action_encoding.md). The compact serializer reuses it
// to mark padded legal-action slots so the input layout never collides
// with any real `(from, to)` pair in `[0, kBoardCells)^2`.
constexpr float kCompactActionPadValue = static_cast<float>(kBoardCells);

bool LegalActionFeatureLess(const LegalActionFeature& lhs,
                            const LegalActionFeature& rhs) noexcept {
  if (lhs.canonical_action.from != rhs.canonical_action.from) {
    return lhs.canonical_action.from < rhs.canonical_action.from;
  }
  return lhs.canonical_action.to < rhs.canonical_action.to;
}

std::vector<float> SerializeBoardState(const XqGame& game) noexcept {
  // Layout is plane-major:
  //   planes  0..6: own-piece planes (General .. Soldier)
  //   planes  7..13: opponent-piece planes (same order)
  //   plane   14:    side-to-move plane (all 1s if Red to move, else 0)
  // Each plane holds 90 cells in row-major (`r * 9 + c`) order.
  std::vector<float> out(kDenseStateFeatureSize, 0.0F);

  // Use the canonical board so own pieces are positive (codes 1..7)
  // and opponent pieces are negative (codes -1..-7).
  const XqB canonical = game.CanonicalBoard();
  for (size_t cell = 0; cell < kBoardCells; ++cell) {
    const int8_t code = canonical[cell];
    if (code == 0) continue;
    const size_t plane = IsRed(code) ? static_cast<size_t>(code - 1)
                                     : static_cast<size_t>(7 + (-code - 1));
    out[PlaneOffset(plane) + cell] = 1.0F;
  }

  if (game.CurrentPlayer() == kRed) {
    const size_t base = PlaneOffset(14);
    std::fill(out.begin() + base, out.begin() + base + kBoardCells, 1.0F);
  }

  return out;
}

std::vector<LegalActionFeature> SortedLegalActionFeatures(
    const XqGame& game) noexcept {
  std::array<XqA, XqGame::kMaxLegalActions> actions{};
  const size_t count = game.ValidActionsInto(actions);
  std::vector<LegalActionFeature> features;
  features.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    const XqA canonical_action = game.CanonicalAction(actions[i]);
    features.push_back(LegalActionFeature{
        canonical_action, game.PolicyIndex(canonical_action), i});
  }
  std::sort(features.begin(), features.end(), LegalActionFeatureLess);
  return features;
}

void AppendCompactActionFeatures(const XqGame& game,
                                 std::vector<float>& out) noexcept {
  const std::vector<LegalActionFeature> features =
      SortedLegalActionFeatures(game);
  for (size_t i = 0; i < XqGame::kMaxLegalActions; ++i) {
    if (i < features.size()) {
      out.push_back(static_cast<float>(features[i].canonical_action.from));
      out.push_back(static_cast<float>(features[i].canonical_action.to));
    } else {
      out.push_back(kCompactActionPadValue);
      out.push_back(kCompactActionPadValue);
    }
  }
}

void AppendCompactBoardTokens(const XqGame& game,
                              std::vector<float>& out) noexcept {
  // One token per board cell, width = kCompactTokenFeatureWidth. Feature
  // 0 carries the signed piece code; feature 1 is a zero pad so the
  // token width matches the legal-action `(from, to)` tokens.
  const XqB canonical = game.CanonicalBoard();
  for (size_t cell = 0; cell < kBoardCells; ++cell) {
    out.push_back(static_cast<float>(canonical[cell]));
    out.push_back(0.0F);
  }
}

void AppendRepeatCounter(const XqGame& game, std::vector<float>& out) noexcept {
  // Single token: feature 0 holds the repeat count, feature 1 is a zero
  // pad to keep `kCompactTokenFeatureWidth` uniform across the sequence.
  out.push_back(static_cast<float>(game.CurrentPositionRepeatCount()));
  out.push_back(0.0F);
}

}  // namespace

std::vector<float> XqSerializer::SerializeCurrentState(
    const XqGame& game,
    ::az::game::api::RingBufferView<XqGame> /*history*/) const noexcept {
  // Markov game: history view is unused.
  return SerializeBoardState(game);
}

std::vector<float> XqSerializer::SerializePolicyOutput(
    const XqGame& game,
    const ::az::game::api::TrainingTarget& target) const noexcept {
  // Canonical layout shared with `XqDeserializer`:
  //   out[0]                                       = target.z
  //   out[1 + PolicyIndex(CanonicalAction(a_i))]   = target.pi[i]
  //   other slots                                  = 0
  //
  // Slots live in the same canonical frame as `CanonicalBoard()`, so
  // Red and Black share the policy-head row that encodes the same
  // canonical move (e.g., "own piece at canonical cell X advances to
  // cell Y"), rather than indexing into disjoint halves of W_head.
  std::vector<float> out(XqGame::kPolicySize + 1, 0.0F);
  out[0] = target.z;

  std::array<XqA, XqGame::kMaxLegalActions> actions{};
  const size_t count = game.ValidActionsInto(actions);
  const size_t n = std::min(count, target.pi.size());
  for (size_t i = 0; i < n; ++i) {
    const size_t slot = 1 + game.PolicyIndex(game.CanonicalAction(actions[i]));
    if (slot < out.size()) {
      out[slot] = target.pi[i];
    }
  }
  return out;
}

std::vector<float> XqCompactSerializer::SerializeCurrentState(
    const XqGame& game,
    ::az::game::api::RingBufferView<XqGame> /*history*/) const noexcept {
  // Markov game: history view is unused. The compact layout is a flat
  // concatenation of `kCompactStateTokenCount = 195` tokens, each
  // `kCompactTokenFeatureWidth = 2` floats wide:
  //   tokens 0..89  : board cells,   [piece_code, 0]
  //   token  90     : repeat counter, [repeat_count, 0]
  //   tokens 91..194: action slots,  [from, to]; padding slots use
  //                                   {kBoardCells, kBoardCells} sentinel.
  std::vector<float> out;
  out.reserve(kCompactStateFeatureSize);
  AppendCompactBoardTokens(game, out);
  AppendRepeatCounter(game, out);
  AppendCompactActionFeatures(game, out);
  return out;
}

::az::game::api::CompactPolicyTargetBlob
XqCompactSerializer::SerializePolicyOutput(
    const XqGame& game,
    const ::az::game::api::TrainingTarget& target) const noexcept {
  const std::vector<LegalActionFeature> features =
      SortedLegalActionFeatures(game);
  ::az::game::api::CompactPolicyTargetBlob blob;
  blob.value = target.z;
  blob.count = features.size();
  blob.legal_indices.reserve(XqGame::kMaxLegalActions);
  blob.values.reserve(XqGame::kMaxLegalActions);

  for (size_t i = 0; i < features.size(); ++i) {
    const size_t target_index = features[i].valid_action_index;
    const float value =
        target_index < target.pi.size() ? target.pi[target_index] : 0.0F;
    blob.legal_indices.push_back(features[i].policy_index);
    blob.values.push_back(value);
  }

  for (size_t i = features.size(); i < XqGame::kMaxLegalActions; ++i) {
    blob.legal_indices.push_back(
        ::az::game::api::CompactPolicyTargetBlob::kPaddingSlot);
    blob.values.push_back(0.0F);
  }

  return blob;
}

}  // namespace az::game::xq
