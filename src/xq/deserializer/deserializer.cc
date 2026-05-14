#include "include/xq/deserializer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

namespace {

using ::az::game::api::CompactPolicyTargetBlob;

}  // namespace

XqResult<::az::game::api::Evaluation> XqDeserializer::Deserialize(
    const XqGame& game, std::span<const float> output) const noexcept {
  // Layout (mirrors XqSerializer::SerializePolicyOutput):
  //   output[0]                                     = value scalar
  //   output[1 + PolicyIndex(CanonicalAction(a))]   = prior for `a`
  // Total length: kPolicySize + 1. Slot lookups go through the
  // canonical frame so Red and Black share the same policy-head row
  // for equivalent canonical moves.
  constexpr size_t kExpected = XqGame::kPolicySize + 1;
  if (output.size() != kExpected) {
    return std::unexpected(XqError::kInvalidPolicyOutputSize);
  }

  std::array<XqA, XqGame::kMaxLegalActions> actions{};
  const size_t count = game.ValidActionsInto(actions);
  std::vector<float> probs;
  probs.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    probs.push_back(
        output[1 + game.PolicyIndex(game.CanonicalAction(actions[i]))]);
  }
  return ::az::game::api::Evaluation{output.front(), std::move(probs)};
}

XqResult<::az::game::api::Evaluation> XqCompactDeserializer::Deserialize(
    const XqGame& game,
    const ::az::game::api::CompactPolicyOutputBlob& output) const noexcept {
  if (output.legal_indices.size() != output.values.size() ||
      output.legal_indices.size() > XqGame::kMaxLegalActions) {
    return std::unexpected(XqError::kInvalidPolicyOutputSize);
  }

  std::array<XqA, XqGame::kMaxLegalActions> actions{};
  const size_t count = game.ValidActionsInto(actions);
  std::array<uint8_t, XqGame::kMaxLegalActions> matched{};
  std::vector<float> probs(count, 0.0F);
  size_t non_padding_count = 0;

  for (size_t j = 0; j < output.legal_indices.size(); ++j) {
    if (output.legal_indices[j] == CompactPolicyTargetBlob::kPaddingSlot) {
      continue;
    }
    ++non_padding_count;
    bool found = false;
    for (size_t i = 0; i < count; ++i) {
      const size_t slot = game.PolicyIndex(game.CanonicalAction(actions[i]));
      if (output.legal_indices[j] == slot) {
        if (matched[i] != 0) {
          return std::unexpected(XqError::kInvalidPolicyOutputSize);
        }
        probs[i] = output.values[j];
        matched[i] = 1;
        found = true;
        break;
      }
    }
    if (!found) {
      return std::unexpected(XqError::kInvalidPolicyOutputSize);
    }
  }

  if (non_padding_count != count) {
    return std::unexpected(XqError::kInvalidPolicyOutputSize);
  }

  return ::az::game::api::Evaluation{output.value, std::move(probs)};
}

}  // namespace az::game::xq
