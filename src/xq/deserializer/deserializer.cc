#include "include/xq/deserializer.h"

#include <array>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include "alpha-zero-api/policy_output.h"
#include "include/xq/game.h"

namespace az::game::xq {

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

}  // namespace az::game::xq
